#include "LxCharacterAttributeComponent.h"

#include "LxARPG/LxSource/Core/Tools/LxAttributeValueTool.h"
#include "LxARPG/LxSource/Core/Tools/LxRichTextDescriptionTool.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxCharacterEntryComponent.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxItemEntryLogic.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "../DataType/LxAttributeData.h"

namespace
{
	const FLxEntryValueInfo* GetAttributeEntryValueInfo(const FLxItemEntryData& InEntryData)
	{
		if (InEntryData.EnteryBaseInfo.EntryLogicType == ELxEntryLogicType::ChangeAttributeValue)
		{
			return &InEntryData.ChangeAttributeValue.EntryValueInfo;
		}

		if (InEntryData.EnteryBaseInfo.EntryLogicType == ELxEntryLogicType::BufferEnter
			&& InEntryData.BufferEnterValue.EntryValueInfo.EffectivePeriod < 0.f)
		{
			return &InEntryData.BufferEnterValue.EntryValueInfo;
		}

		return nullptr;
	}

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

FText ULxCharacterAttributeComponent::BuildAttributeDisplayText(const FLxAttributeData& InAttributeData)
{
	FLxRichTextDescriptionGroupData AttributeNameData = InAttributeData.AttributeShowInfo.AttributeName;
	FLxString outText = FLxRichTextDescriptionTool::MontageRichText(AttributeNameData);

	outText.Arg(FLxAttributeValueTool::BuildAttributeValueText(InAttributeData));

	return outText.ToFText();
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
		EntryComponent->OnAttributeRecoveryEntryApplied.RemoveDynamic(this, &ULxCharacterAttributeComponent::HandleAttributeRecoveryEntryApplied);
		EntryComponent->OnAttributeRecoveryEntryApplied.AddDynamic(this, &ULxCharacterAttributeComponent::HandleAttributeRecoveryEntryApplied);
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

void ULxCharacterAttributeComponent::HandleAttributeRecoveryEntryApplied(ULxItemEntryLogic* EntryLogic)
{
	if (EntryLogic == nullptr || !EntryLogic->IsEntryValid())
	{
		return;
	}

	const FLxItemEntryData& EntryData = EntryLogic->GetItemEntryData();
	if (EntryData.EnteryBaseInfo.TargetTags.IsEmpty())
	{
		return;
	}

	const float RestoreValue = EntryLogic->GetEffectiveValue();
	if (FMath::IsNearlyZero(RestoreValue))
	{
		return;
	}

	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		if (AttributeMatchesTargetTags(AttributePair.Value, EntryData.EnteryBaseInfo.TargetTags))
		{
			RestoreCharacterAttributeCurrentValue(AttributePair.Key, RestoreValue);
		}
	}
}

void ULxCharacterAttributeComponent::HandleEntryPackageChanged(const FLxCharacterEntryPackage& InEntryPackage)
{
	// 只缓存属性相关词条，其他类别交给各自系统消费。
	m_vCharacterAttributeEntries = InEntryPackage.CharacterAttributeEntryList;
}

void ULxCharacterAttributeComponent::RefreshCharacterAttributeByEntries()
{

	// 每次重算前都先回退到属性基础值，确保装备/词条效果不会在旧结果上重复累加。
	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{

		AttributePair.Value.CalculatedAttributeValue.ValueLimit = AttributePair.Value.AttributeValue.ValueLimit;
		AttributePair.Value.CalculatedAttributeValue.UpwardFloatingRatio = AttributePair.Value.AttributeValue.UpwardFloatingRatio;
		AttributePair.Value.CalculatedAttributeValue.DownwardFloatingRatio = AttributePair.Value.AttributeValue.DownwardFloatingRatio;
		AttributePair.Value.CalculatedAttributeValue.ValueType = AttributePair.Value.AttributeValue.ValueType;
		
		// 如果是区间值类型，就不用重置有效值
		if (AttributePair.Value.CalculatedAttributeValue.ValueType != ELxCharacterValueType::RangedNumeric)
		{
			AttributePair.Value.CalculatedAttributeValue.Value = AttributePair.Value.AttributeValue.Value;
		}

	}

	// 按固定顺序结算词条，保证基础值、基础加成、额外加成、机制类效果的叠加顺序稳定。
	const ELxItemEntryType EntryApplyOrder[] =
	{
		ELxItemEntryType::BasicValue,
		ELxItemEntryType::BasicImprove,
		ELxItemEntryType::AdditionalImprove,
		ELxItemEntryType::Mechanism,
	};

	// 逐阶段遍历角色当前持有的属性词条，只处理“修改属性值”这一类词条。
	for (const ELxItemEntryType EntryType : EntryApplyOrder)
	{
		for (ULxItemEntryLogic* EntryLogic : m_vCharacterAttributeEntries)
		{
			if (EntryLogic == nullptr || !EntryLogic->IsEntryValid())
			{
				continue;
			}

			const FLxItemEntryData& EntryData = EntryLogic->GetItemEntryData();
			const FLxEntryValueInfo* EntryValueInfo = GetAttributeEntryValueInfo(EntryData);
			if (EntryData.EnteryBaseInfo.TargetTags.IsEmpty()
				|| EntryValueInfo == nullptr
				|| EntryValueInfo->EntryType != EntryType)
			{
				continue;
			}

			// 将词条作用到所有命中目标标签的属性上，实现同一词条同时影响多种属性。
			for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
			{
				if (AttributeMatchesTargetTags(AttributePair.Value, EntryData.EnteryBaseInfo.TargetTags))
				{
					ApplyEntryToAttribute(AttributePair.Value, *EntryLogic);
				}
			}
		}
	}

	// 在派生属性计算前先整理一次结果，避免后续规则建立在未归一化的中间值上。
	// for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	// {
	// 	NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	// }

	// 基础属性和词条结果稳定后，再统一刷新属性间的派生加成关系。
	RefreshDerivedAttributes();

	// 派生属性可能再次改变最终数值范围，因此最后再做一次归一化收口。
	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
}

void ULxCharacterAttributeComponent::RefreshDerivedAttributes()
{
	TMap<ELxCharacterAttributeID, float> SourceValueSnapshot;
	SourceValueSnapshot.Reserve(m_mapCharacterAttributeTable.Num());

	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : m_mapCharacterAttributeTable)
	{
		SourceValueSnapshot.Add(AttributePair.Key, AttributePair.Value.CalculatedAttributeValue.Value);
	}

	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& SourcePair : m_mapCharacterAttributeTable)
	{
		const float* SourceValue = SourceValueSnapshot.Find(SourcePair.Key);
		if (SourceValue == nullptr || FMath::IsNearlyZero(*SourceValue))
		{
			continue;
		}

		for (const FLxAttributeDerivedRule& DerivedRule : SourcePair.Value.DerivedRules)
		{
			if (DerivedRule.TargetTags.IsEmpty() || FMath::IsNearlyZero(DerivedRule.Ratio))
			{
				continue;
			}

			for (TPair<ELxCharacterAttributeID, FLxAttributeData>& TargetPair : m_mapCharacterAttributeTable)
			{
				if (TargetPair.Key == SourcePair.Key)
				{
					continue;
				}

				if (AttributeMatchesTargetTags(TargetPair.Value, DerivedRule.TargetTags))
				{
					ApplyDerivedRuleToAttribute(TargetPair.Value, DerivedRule, *SourceValue);
				}
			}
		}
	}
}

void ULxCharacterAttributeComponent::ApplyEntryToAttribute(FLxAttributeData& InOutAttributeData, const ULxItemEntryLogic& InEntryLogic)
{
	const FLxItemEntryData& EntryData = InEntryLogic.GetItemEntryData();
	const FLxEntryValueInfo* EntryValueInfo = GetAttributeEntryValueInfo(EntryData);
	if (EntryValueInfo == nullptr)
	{
		return;
	}

	FLxAttributeValue& BaseValue = InOutAttributeData.AttributeValue;
	FLxAttributeValue& CalculatedValue = InOutAttributeData.CalculatedAttributeValue;

	const float EffectiveValue = InEntryLogic.GetEffectiveValue();
	const float EffectiveRatio = EffectiveValue;

	auto ApplyScalarValue = [EffectiveValue, EffectiveRatio](float& InOutTargetValue, float InBaseValue, ELxItemEntryType InEntryType)
	{
		switch (InEntryType)
		{
		case ELxItemEntryType::BasicValue:
			InOutTargetValue += EffectiveValue;
			break;
		case ELxItemEntryType::BasicImprove:
		case ELxItemEntryType::AdditionalImprove:
			InOutTargetValue += InOutTargetValue * EffectiveRatio;
			break;
		case ELxItemEntryType::Mechanism:
			InOutTargetValue = FMath::Max(InOutTargetValue, EffectiveValue);
			break;
		}
	};

	switch (EntryValueInfo->EntryTarget)
	{
	case ELxItemEntryTarget::ToValueLimit:
		ApplyScalarValue(CalculatedValue.ValueLimit, BaseValue.ValueLimit, EntryValueInfo->EntryType);
		break;
	case ELxItemEntryTarget::ToValue:
		ApplyScalarValue(CalculatedValue.Value, BaseValue.Value, EntryValueInfo->EntryType);
		break;
	case ELxItemEntryTarget::ToUpwardFloatingRatio:
		ApplyScalarValue(CalculatedValue.UpwardFloatingRatio, BaseValue.UpwardFloatingRatio, EntryValueInfo->EntryType);
		break;
	case ELxItemEntryTarget::ToDownwardFloatingRatio:
		ApplyScalarValue(CalculatedValue.DownwardFloatingRatio, BaseValue.DownwardFloatingRatio, EntryValueInfo->EntryType);
		break;
	}
}

void ULxCharacterAttributeComponent::ApplyDerivedRuleToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeDerivedRule& InDerivedRule, float InSourceValue)
{
	const float DerivedValue = InSourceValue * InDerivedRule.Ratio;
	const float DerivedRatio = InSourceValue * InDerivedRule.Ratio * 0.01f;
	FLxAttributeValue& CalculatedValue = InOutAttributeData.CalculatedAttributeValue;

	auto ApplyDerivedValue = [DerivedValue, DerivedRatio](float& InOutTargetValue, ELxItemEntryType InEntryType)
	{
		switch (InEntryType)
		{
		case ELxItemEntryType::BasicValue:
			InOutTargetValue += DerivedValue;
			break;
		case ELxItemEntryType::BasicImprove:
		case ELxItemEntryType::AdditionalImprove:
			InOutTargetValue += InOutTargetValue * DerivedRatio;
			break;
		case ELxItemEntryType::Mechanism:
			InOutTargetValue = FMath::Max(InOutTargetValue, DerivedValue);
			break;
		}
	};

	switch (InDerivedRule.EntryTarget)
	{
	case ELxItemEntryTarget::ToValueLimit:
		ApplyDerivedValue(CalculatedValue.ValueLimit, InDerivedRule.EntryType);
		break;
	case ELxItemEntryTarget::ToValue:
		ApplyDerivedValue(CalculatedValue.Value, InDerivedRule.EntryType);
		break;
	case ELxItemEntryTarget::ToUpwardFloatingRatio:
		ApplyDerivedValue(CalculatedValue.UpwardFloatingRatio, InDerivedRule.EntryType);
		break;
	case ELxItemEntryTarget::ToDownwardFloatingRatio:
		ApplyDerivedValue(CalculatedValue.DownwardFloatingRatio, InDerivedRule.EntryType);
		break;
	}
}

bool ULxCharacterAttributeComponent::AttributeMatchesTargetTags(const FLxAttributeData& InAttributeData, const FGameplayTagContainer& InTargetTags)
{
	if (InTargetTags.IsEmpty())
	{
		return false;
	}

	return InAttributeData.AttributeInfo.AttributeTags.HasAll(InTargetTags);
}
