#include "LxCharacterAttributeComponent.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxCharacterEntryComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "../DataType/LxAttributeData.h"

namespace
{
	bool IsDiscreteAttributeValueType(ELxCharacterValueType InValueType)
	{
		return InValueType == ELxCharacterValueType::FixedNumeric
			|| InValueType == ELxCharacterValueType::RangedNumeric
			|| InValueType == ELxCharacterValueType::FloatingNumeric
			|| InValueType == ELxCharacterValueType::Switch
			|| InValueType == ELxCharacterValueType::Setting;
	}

	void NormalizeAttributeValueByType(FLxAttributeValue& InOutAttributeValue)
	{
		if (IsDiscreteAttributeValueType(InOutAttributeValue.ValueType))
		{
			InOutAttributeValue.ValueLimit = FMath::RoundToFloat(InOutAttributeValue.ValueLimit);
			InOutAttributeValue.Value = FMath::RoundToFloat(InOutAttributeValue.Value);
		}

		if (InOutAttributeValue.ValueType == ELxCharacterValueType::RangedNumeric)
		{
			InOutAttributeValue.ValueLimit = FMath::Max(0.0f, InOutAttributeValue.ValueLimit);
			InOutAttributeValue.Value = FMath::Clamp(InOutAttributeValue.Value, 0.0f, InOutAttributeValue.ValueLimit);
		}
	}
}

ULxCharacterAttributeComponent::ULxCharacterAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterAttributeComponent::BaseComponentInitialize()
{
	if (m_bAttributeInitialized)
	{
		return;
	}

	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = GetCharacterOwner();
	}

	ULxGameInstanceSubsystem* GameInstanceSubsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	if (!GameInstanceSubsystem)
	{
		return;
	}

	const ULxGameDataTablesManager* GameDataTablesManager = GameInstanceSubsystem->GetGameDataManager();
	if (!GameDataTablesManager)
	{
		return;
	}

	const ULxAttributeTableConfig* AttributeTableConfig = GameDataTablesManager->m_pCharacterAttributeTableConfig;
	if (!AttributeTableConfig)
	{
		return;
	}

	const TArray<FLxAttributeData>* AttributeDataList = AttributeTableConfig->GetAttributeDataList(CharacterRaceType);
	if (!AttributeDataList)
	{
		return;
	}

	m_mapCharacterAttributeTable.Empty();
	for (const FLxAttributeData& AttributeData : *AttributeDataList)
	{
		if (AttributeData.AttributeInfo.AttributeID == ELxCharacterAttributeID::X_None)
		{
			continue;
		}

		m_mapCharacterAttributeTable.Add(AttributeData.AttributeInfo.AttributeID, AttributeData);
	}

	if (ULxCharacterEntryComponent* EntryComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEntryComponent() : nullptr)
	{
		// 属性组件不再主动扫描装备/背包，只接收词条组件统一分发的结果。
		EntryComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterAttributeComponent::HandleEntryDataChange);
		EntryComponent->OnDataChange.AddDynamic(this, &ULxCharacterAttributeComponent::HandleEntryDataChange);
		EntryComponent->OnInstantRestoreEntryApplied.RemoveDynamic(this, &ULxCharacterAttributeComponent::HandleInstantRestoreEntryApplied);
		EntryComponent->OnInstantRestoreEntryApplied.AddDynamic(this, &ULxCharacterAttributeComponent::HandleInstantRestoreEntryApplied);
		EntryComponent->OnEntryPackageChanged.RemoveDynamic(this, &ULxCharacterAttributeComponent::HandleEntryPackageChanged);
		EntryComponent->OnEntryPackageChanged.AddDynamic(this, &ULxCharacterAttributeComponent::HandleEntryPackageChanged);
		m_vCharacterAttributeEntries = EntryComponent->GetEntryPackage().CharacterAttributeEntryList;
	}

	m_bAttributeInitialized = true;
	RefreshCharacterAttributeByEntries();
	OnDataChange.Broadcast();
}

FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const ELxCharacterAttributeID InAttributeID)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

const FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const ELxCharacterAttributeID InAttributeID) const
{
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

TMap<ELxCharacterAttributeID, FLxAttributeData>* ULxCharacterAttributeComponent::GetCharacterAttributeTable()
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return &m_mapCharacterAttributeTable;
}

const TMap<ELxCharacterAttributeID, FLxAttributeData>* ULxCharacterAttributeComponent::GetCharacterAttributeTable() const
{
	return &m_mapCharacterAttributeTable;
}

bool ULxCharacterAttributeComponent::SetCharacterAttribute(const ELxCharacterAttributeID InAttributeID, const FLxAttributeData& InAttributeData)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	return true;
}

bool ULxCharacterAttributeComponent::SetCharacterAttributeCurrentValue(const ELxCharacterAttributeID InAttributeID, float InNewValue)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	FLxAttributeData* AttributeData = m_mapCharacterAttributeTable.Find(InAttributeID);
	if (!AttributeData)
	{
		return false;
	}

	AttributeData->CalculatedAttributeValue.Value = InNewValue;
	NormalizeAttributeValueByType(AttributeData->CalculatedAttributeValue);
	OnDataChange.Broadcast();
	return true;
}

bool ULxCharacterAttributeComponent::RestoreCharacterAttributeCurrentValue(const ELxCharacterAttributeID InAttributeID, float InRestoreValue)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	const FLxAttributeData* AttributeData = GetCharacterAttributeByID(InAttributeID);
	if (AttributeData == nullptr)
	{
		return false;
	}

	return SetCharacterAttributeCurrentValue(InAttributeID, AttributeData->CalculatedAttributeValue.Value + InRestoreValue);
}

void ULxCharacterAttributeComponent::HandleEntryDataChange()
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
		return;
	}

	if (const ULxCharacterEntryComponent* EntryComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEntryComponent() : nullptr)
	{
		// OnDataChange 可能先于 OnEntryPackageChanged 到达，这里主动同步一次最新打包结果，
		// 避免属性刷新时仍然使用上一轮词条缓存。
		m_vCharacterAttributeEntries = EntryComponent->GetEntryPackage().CharacterAttributeEntryList;
	}

	RefreshCharacterAttributeByEntries();
	OnDataChange.Broadcast();
}

void ULxCharacterAttributeComponent::HandleInstantRestoreEntryApplied(ELxCharacterAttributeID InAttributeID, float InRestoreValue)
{
	RestoreCharacterAttributeCurrentValue(InAttributeID, InRestoreValue);
}

void ULxCharacterAttributeComponent::HandleEntryPackageChanged(const FLxCharacterEntryPackage& InEntryPackage)
{
	// 只缓存属性相关词条，其他类别交给各自系统消费。
	m_vCharacterAttributeEntries = InEntryPackage.CharacterAttributeEntryList;
}

void ULxCharacterAttributeComponent::RefreshCharacterAttributeByEntries()
{
	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		AttributePair.Value.CalculatedAttributeValue = AttributePair.Value.AttributeValue;
	}

	for (const FLxItemEntryData& EntryData : m_vCharacterAttributeEntries)
	{
		FLxAttributeData* AttributeData = m_mapCharacterAttributeTable.Find(EntryData.AttributeID);
		if (AttributeData == nullptr)
		{
			continue;
		}

		ApplyEntryToAttribute(*AttributeData, EntryData);
	}

	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
}

void ULxCharacterAttributeComponent::ApplyEntryToAttribute(FLxAttributeData& InOutAttributeData, const FLxItemEntryData& InEntryData)
{
	if (InEntryData.AttributeID == ELxCharacterAttributeID::X_None)
	{
		return;
	}

	FLxAttributeValue& BaseValue = InOutAttributeData.AttributeValue;
	FLxAttributeValue& CalculatedValue = InOutAttributeData.CalculatedAttributeValue;

	const float EffectiveValue = InEntryData.ItemEntryDefineValue.Value * InEntryData.EffectiveRatio;
	const float EffectiveRatio = EffectiveValue;

	auto ApplyScalarValue = [EffectiveValue, EffectiveRatio](float& InOutTargetValue, float InBaseValue, ELxItemEntryType InEntryType)
	{
		switch (InEntryType)
		{
		case ELxItemEntryType::BasicValue:
			InOutTargetValue += EffectiveValue;
			break;
		case ELxItemEntryType::BasicImprove:
			InOutTargetValue += InBaseValue * EffectiveRatio;
			break;
		case ELxItemEntryType::AdditionalImprove:
			InOutTargetValue += InOutTargetValue * EffectiveRatio;
			break;
		case ELxItemEntryType::Mechanism:
			InOutTargetValue = FMath::Max(InOutTargetValue, EffectiveValue);
			break;
		}
	};

	switch (InEntryData.ItemEntryDefineValue.EntryTarget)
	{
	case ELxItemEntryTarget::ToValueLimit:
		ApplyScalarValue(CalculatedValue.ValueLimit, BaseValue.ValueLimit, InEntryData.ItemEntryDefineValue.EntryType);
		break;
	case ELxItemEntryTarget::ToValue:
		ApplyScalarValue(CalculatedValue.Value, BaseValue.Value, InEntryData.ItemEntryDefineValue.EntryType);
		break;
	case ELxItemEntryTarget::ToUpwardFloatingRatio:
		ApplyScalarValue(CalculatedValue.UpwardFloatingRatio, BaseValue.UpwardFloatingRatio, InEntryData.ItemEntryDefineValue.EntryType);
		break;
	case ELxItemEntryTarget::ToDownwardFloatingRatio:
		ApplyScalarValue(CalculatedValue.DownwardFloatingRatio, BaseValue.DownwardFloatingRatio, InEntryData.ItemEntryDefineValue.EntryType);
		break;
	}
}
