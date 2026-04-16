#include "LxCharacterAttributeComponent.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
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
		if (AttributeData.AttributeInfo.AttributeID.IsNone())
		{
			continue;
		}

		m_mapCharacterAttributeTable.Add(AttributeData.AttributeInfo.AttributeID, AttributeData);
	}

	if (ULxCharacterEquipmentComponent* EquipmentComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEquipmentComponent() : nullptr)
	{
		EquipmentComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterAttributeComponent::HandleEquipmentDataChange);
		EquipmentComponent->OnDataChange.AddDynamic(this, &ULxCharacterAttributeComponent::HandleEquipmentDataChange);
	}

	m_bAttributeInitialized = true;
	RefreshCharacterAttributeByEquipment();
	OnDataChange.Broadcast();
}

FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const FName& InAttributeID)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

const FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(const FName& InAttributeID) const
{
	return m_mapCharacterAttributeTable.Find(InAttributeID);
}

TMap<FName, FLxAttributeData>* ULxCharacterAttributeComponent::GetCharacterAttributeTable()
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}
	return &m_mapCharacterAttributeTable;
}

const TMap<FName, FLxAttributeData>* ULxCharacterAttributeComponent::GetCharacterAttributeTable() const
{
	return &m_mapCharacterAttributeTable;
}

bool ULxCharacterAttributeComponent::SetCharacterAttribute(const FName& InAttributeID, const FLxAttributeData& InAttributeData)
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
	}

	return true;
}

bool ULxCharacterAttributeComponent::SetCharacterAttributeCurrentValue(const FName& InAttributeID, float InNewValue)
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
	return true;
}

void ULxCharacterAttributeComponent::HandleEquipmentDataChange()
{
	if (!m_bAttributeInitialized)
	{
		BaseComponentInitialize();
		return;
	}

	RefreshCharacterAttributeByEquipment();
	OnDataChange.Broadcast();
}

void ULxCharacterAttributeComponent::RefreshCharacterAttributeByEquipment()
{
	for (TPair<FName, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		AttributePair.Value.CalculatedAttributeValue = AttributePair.Value.AttributeValue;
	}

	TArray<ULxEquipmentLogic*> EquipmentList;
	GetAllEquipmentList(EquipmentList);
	for (ULxEquipmentLogic* EquipmentLogic : EquipmentList)
	{
		if (EquipmentLogic == nullptr || !EquipmentLogic->ItemIsValid())
		{
			continue;
		}

		const FLxEquipmentData* EquipmentData = EquipmentLogic->GetEquipmentData();
		if (EquipmentData == nullptr)
		{
			continue;
		}

		if (FLxAttributeData* BasicAttributeData = m_mapCharacterAttributeTable.Find(EquipmentData->EquipmentEntyInfo.EquipmentBasicEntry.AttributeID))
		{
			ApplyEquipmentEntryToAttribute(*BasicAttributeData, EquipmentData->EquipmentEntyInfo.EquipmentBasicEntry);
		}

		for (const FLxItemEntryData& EntryData : EquipmentData->EquipmentEntyInfo.EquipmentExtendEntryList)
		{
			FLxAttributeData* AttributeData = m_mapCharacterAttributeTable.Find(EntryData.AttributeID);
			if (AttributeData == nullptr)
			{
				continue;
			}

			ApplyEquipmentEntryToAttribute(*AttributeData, EntryData);
		}
	}

	for (TPair<FName, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
}

void ULxCharacterAttributeComponent::GetAllEquipmentList(TArray<ULxEquipmentLogic*>& OutEquipmentList) const
{
	OutEquipmentList.Reset();

	ULxCharacterEquipmentComponent* EquipmentComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEquipmentComponent() : nullptr;
	if (EquipmentComponent == nullptr)
	{
		return;
	}

	for (ULxEquipmentSlotData* EquipmentSlot : EquipmentComponent->GetEquipmentSlots())
	{
		if (EquipmentSlot == nullptr || !EquipmentSlot->IsValid())
		{
			continue;
		}

		ULxEquipmentLogic* EquipmentLogic = Cast<ULxEquipmentLogic>(EquipmentSlot->ItemDataPtr);
		if (EquipmentLogic == nullptr || !EquipmentLogic->ItemIsValid())
		{
			continue;
		}

		OutEquipmentList.Add(EquipmentLogic);
	}
}

void ULxCharacterAttributeComponent::ApplyEquipmentEntryToAttribute(FLxAttributeData& InOutAttributeData, const FLxItemEntryData& InEntryData)
{
	if (InEntryData.AttributeID.IsNone())
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
