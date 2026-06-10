#include "LxCharacterAttributeComponent.h"

#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"

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

	bool LoadAttributeValueConfigsFromTable(const UDataTable* InAttributeValueTable, TArray<FLxAttributeValueConfig>& OutValueConfigs)
	{
		OutValueConfigs.Reset();
		if (InAttributeValueTable == nullptr)
		{
			return true;
		}

		if (InAttributeValueTable->GetRowStruct() != FLxAttributeValueConfig::StaticStruct())
		{
			return false;
		}

		TArray<FLxAttributeValueConfig*> Rows;
		InAttributeValueTable->GetAllRows<FLxAttributeValueConfig>(TEXT("ULxCharacterAttributeComponent::LoadAttributeValueConfigsFromTable"), Rows);
		OutValueConfigs.Reserve(Rows.Num());

		for (const FLxAttributeValueConfig* RowData : Rows)
		{
			if (RowData == nullptr)
			{
				continue;
			}

			OutValueConfigs.Add(*RowData);
		}

		return true;
	}

	ELxCharacterAttributeID ResolveAttributeID(const TMap<ELxCharacterAttributeID, FLxAttributeData>& InAttributeDataMap, const FLxAttributeValueConfig& InValueConfig)
	{
		if (InValueConfig.AttributeID != ELxCharacterAttributeID::X_None)
		{
			return InValueConfig.AttributeID;
		}

		if (!InValueConfig.AttributeIDTag.IsValid())
		{
			return ELxCharacterAttributeID::X_None;
		}

		for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : InAttributeDataMap)
		{
			if (AttributePair.Value.AttributeIDTag == InValueConfig.AttributeIDTag)
			{
				return AttributePair.Key;
			}
		}

		return ELxCharacterAttributeID::X_None;
	}

	void ApplyBaseValueConfigToAttribute(const FLxAttributeValueConfig& InValueConfig, FLxAttributeData& InOutAttributeData)
	{
		if (InValueConfig.AttributeID != ELxCharacterAttributeID::X_None)
		{
			InOutAttributeData.AttributeID = InValueConfig.AttributeID;
		}
		if (InValueConfig.AttributeIDTag.IsValid())
		{
			InOutAttributeData.AttributeIDTag = InValueConfig.AttributeIDTag;
		}

		InOutAttributeData.AttributeValue.UpwardFloatingRatio = InValueConfig.UpwardFloatingRatio;
		InOutAttributeData.AttributeValue.DownwardFloatingRatio = InValueConfig.DownwardFloatingRatio;
		InOutAttributeData.AttributeValue.ValueLimit = InValueConfig.ValueLimit;
		InOutAttributeData.AttributeValue.Value = InValueConfig.Value;
		InOutAttributeData.CalculatedAttributeValue = InOutAttributeData.AttributeValue;
	}

	void ApplyBaseValueConfigsToAttributeMap(const TArray<FLxAttributeValueConfig>& InValueConfigs, TMap<ELxCharacterAttributeID, FLxAttributeData>& InOutAttributeDataMap)
	{
		for (const FLxAttributeValueConfig& ValueConfig : InValueConfigs)
		{
			const ELxCharacterAttributeID AttributeID = ResolveAttributeID(InOutAttributeDataMap, ValueConfig);
			if (AttributeID == ELxCharacterAttributeID::X_None)
			{
				continue;
			}

			FLxAttributeData* FoundAttributeData = InOutAttributeDataMap.Find(AttributeID);
			if (FoundAttributeData == nullptr)
			{
				FLxAttributeData NewData;
				ApplyBaseValueConfigToAttribute(ValueConfig, NewData);
				if (NewData.AttributeID == ELxCharacterAttributeID::X_None)
				{
					NewData.AttributeID = AttributeID;
				}
				InOutAttributeDataMap.Add(AttributeID, NewData);
				continue;
			}

			ApplyBaseValueConfigToAttribute(ValueConfig, *FoundAttributeData);
		}
	}
}

ULxCharacterAttributeComponent::ULxCharacterAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterAttributeComponent::BaseComponentInitialize()
{
	if (CharacterAttributeValueTable != nullptr)
	{
		LoadAttributeValueConfigsFromTable(CharacterAttributeValueTable, CharacterAttributeValueConfigs);
	}

	InitializeAttributeTable();
	BroadcastAttributeTableChanged();
}

bool ULxCharacterAttributeComponent::SetCharacterAttributeValueTable(UDataTable* InAttributeValueTable, bool bReinitializeAttribute)
{
	TArray<FLxAttributeValueConfig> LoadedValueConfigs;
	if (!LoadAttributeValueConfigsFromTable(InAttributeValueTable, LoadedValueConfigs))
	{
		return false;
	}

	CharacterAttributeValueTable = InAttributeValueTable;
	CharacterAttributeValueConfigs = LoadedValueConfigs;

	if (bReinitializeAttribute)
	{
		InitializeAttributeTable();
		BroadcastAttributeTableChanged();
	}

	return true;
}

void ULxCharacterAttributeComponent::ReceiveAttributeGainEntries(ELxCharacterAttributeEntrySource InEntrySource, const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList)
{
	if (CharacterAttributeTable.IsEmpty())
	{
		InitializeAttributeTable();
	}

	if (InEntrySource == ELxCharacterAttributeEntrySource::None)
	{
		InEntrySource = ELxCharacterAttributeEntrySource::Other;
	}

	AttributeGainEntryCache.Add(InEntrySource, InEntryList);
	RefreshCharacterAttributesByCachedEntries();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::ReceiveAttributeRecoveryEntries(const TArray<TObjectPtr<ULxEntryObjectBase>>& InEntryList)
{
	if (CharacterAttributeTable.IsEmpty())
	{
		InitializeAttributeTable();
	}

	for (ULxEntryObjectBase* EntryObject : InEntryList)
	{
		if (EntryObject == nullptr)
		{
			continue;
		}

		for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
		{
			ApplyEntryToAttribute(AttributePair.Value, *EntryObject);
		}
	}

	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}

	CacheRuntimeRangedAttributeValues();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::GetCharacterAttributeList(TArray<FLxAttributeData>& OutAttributeList) const
{
	OutAttributeList.Reset();
	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		OutAttributeList.Add(AttributePair.Value);
	}
}

const FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByID(ELxCharacterAttributeID InAttributeID) const
{
	return CharacterAttributeTable.Find(InAttributeID);
}

void ULxCharacterAttributeComponent::InitializeAttributeTable()
{
	AttributeGainEntryCache.Empty();
	RuntimeRangedAttributeValues.Empty();
	RebuildAttributeTableFromRaceConfig();
	RefreshDerivedAttributes();
	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
	CacheRuntimeRangedAttributeValues();
}

void ULxCharacterAttributeComponent::RebuildAttributeTableFromRaceConfig()
{
	CharacterAttributeTable.Empty();

	TMap<ELxCharacterAttributeID, FLxAttributeData> AttributeDataMap = CharacterAttributeValueConfigs.IsEmpty()
		? LxAttributeConfig::GetCharacterAttributeDataByRaceType(CharacterRaceType)
		: LxAttributeConfig::GetAttributeDataMap();
	if (!CharacterAttributeValueConfigs.IsEmpty())
	{
		ApplyBaseValueConfigsToAttributeMap(CharacterAttributeValueConfigs, AttributeDataMap);
	}

	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : AttributeDataMap)
	{
		const FLxAttributeData& AttributeData = AttributePair.Value;
		if (AttributeData.AttributeID == ELxCharacterAttributeID::X_None)
		{
			continue;
		}

		CharacterAttributeTable.Add(AttributeData.AttributeID, AttributeData);
	}
}

void ULxCharacterAttributeComponent::RefreshCharacterAttributesByCachedEntries()
{
	CacheRuntimeRangedAttributeValues();
	RebuildAttributeTableFromRaceConfig();

	for (const TPair<ELxCharacterAttributeEntrySource, TArray<TObjectPtr<ULxEntryObjectBase>>>& EntryCachePair : AttributeGainEntryCache)
	{
		for (ULxEntryObjectBase* EntryObject : EntryCachePair.Value)
		{
			if (EntryObject == nullptr)
			{
				continue;
			}

			for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
			{
				ApplyEntryToAttribute(AttributePair.Value, *EntryObject);
			}
		}
	}

	RefreshDerivedAttributes();
	RestoreRuntimeRangedAttributeValues();

	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
	CacheRuntimeRangedAttributeValues();
}

void ULxCharacterAttributeComponent::CacheRuntimeRangedAttributeValues()
{
	RuntimeRangedAttributeValues.Reset();
	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		const FLxAttributeValue& AttributeValue = AttributePair.Value.CalculatedAttributeValue;
		if (AttributeValue.ValueType == ELxCharacterValueType::RangedNumeric)
		{
			RuntimeRangedAttributeValues.Add(AttributePair.Key, AttributeValue.Value);
		}
	}
}

void ULxCharacterAttributeComponent::RestoreRuntimeRangedAttributeValues()
{
	for (TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		FLxAttributeValue& AttributeValue = AttributePair.Value.CalculatedAttributeValue;
		if (AttributeValue.ValueType != ELxCharacterValueType::RangedNumeric)
		{
			continue;
		}

		const float* RuntimeValue = RuntimeRangedAttributeValues.Find(AttributePair.Key);
		if (RuntimeValue != nullptr)
		{
			AttributeValue.Value = *RuntimeValue;
		}
	}
}

void ULxCharacterAttributeComponent::RefreshDerivedAttributes()
{
	TMap<ELxCharacterAttributeID, float> SourceValueSnapshot;
	SourceValueSnapshot.Reserve(CharacterAttributeTable.Num());

	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		SourceValueSnapshot.Add(AttributePair.Key, AttributePair.Value.CalculatedAttributeValue.Value);
	}

	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& SourcePair : CharacterAttributeTable)
	{
		const float* SourceValue = SourceValueSnapshot.Find(SourcePair.Key);
		if (SourceValue == nullptr || FMath::IsNearlyZero(*SourceValue))
		{
			continue;
		}

		for (const FLxAttributeDerivedRule& DerivedRule : SourcePair.Value.DerivedRulesArray)
		{
			if (DerivedRule.AttributeID == ELxCharacterAttributeID::X_None || FMath::IsNearlyZero(DerivedRule.Ratio))
			{
				continue;
			}

			if (DerivedRule.AttributeID == SourcePair.Key)
			{
				continue;
			}

			if (FLxAttributeData* TargetAttributeData = CharacterAttributeTable.Find(DerivedRule.AttributeID))
			{
				ApplyDerivedRuleToAttribute(*TargetAttributeData, DerivedRule, *SourceValue);
			}
		}
	}
}

void ULxCharacterAttributeComponent::BroadcastAttributeTableChanged()
{
	TArray<FLxAttributeData> AttributeList;
	GetCharacterAttributeList(AttributeList);
	OnAttributeTableChanged.Broadcast(AttributeList);
	OnDataChange.Broadcast();
}

void ULxCharacterAttributeComponent::NormalizeAttributeValueByType(FLxAttributeValue& InOutAttributeValue)
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

void ULxCharacterAttributeComponent::ApplyEntryToAttribute(FLxAttributeData& InOutAttributeData, const ULxEntryObjectBase& InEntryObject)
{
	const FLxEntryBase* EntryBase = InEntryObject.GetEntryBase();
	if (EntryBase == nullptr)
	{
		return;
	}

	auto ApplyEntryValue = [](float& InOutTargetValue, ELxEntryEffectiveType InEffectiveType, float InEntryValue)
	{
		switch (InEffectiveType)
		{
		case ELxEntryEffectiveType::BasicValue:
			InOutTargetValue += InEntryValue;
			break;
		case ELxEntryEffectiveType::BasicImprove:
		case ELxEntryEffectiveType::AdditionalImprove:
			InOutTargetValue += InOutTargetValue * InEntryValue * 0.01f;
			break;
		case ELxEntryEffectiveType::Mechanism:
			InOutTargetValue = FMath::Max(InOutTargetValue, InEntryValue);
			break;
		}
	};

	auto ApplyValueToTarget = [&InOutAttributeData, &ApplyEntryValue](ELxEntryTarget InEntryTarget, ELxEntryEffectiveType InEffectiveType, float InEntryValue)
	{
		FLxAttributeValue& AttributeValue = InOutAttributeData.CalculatedAttributeValue;
		switch (InEntryTarget)
		{
		case ELxEntryTarget::ToValueLimit:
			ApplyEntryValue(AttributeValue.ValueLimit, InEffectiveType, InEntryValue);
			break;
		case ELxEntryTarget::ToValue:
			ApplyEntryValue(AttributeValue.Value, InEffectiveType, InEntryValue);
			break;
		case ELxEntryTarget::ToUpwardFloatingRatio:
			ApplyEntryValue(AttributeValue.UpwardFloatingRatio, InEffectiveType, InEntryValue);
			break;
		case ELxEntryTarget::ToDownwardFloatingRatio:
			ApplyEntryValue(AttributeValue.DownwardFloatingRatio, InEffectiveType, InEntryValue);
			break;
		}
	};

	auto ApplyRecoveryValue = [&InOutAttributeData](ELxEntryEffectiveType InEffectiveType, float InEntryValue, float InRecoveryScale)
	{
		FLxAttributeValue& AttributeValue = InOutAttributeData.CalculatedAttributeValue;
		const float ScaledEntryValue = InEntryValue * InRecoveryScale;
		switch (InEffectiveType)
		{
		case ELxEntryEffectiveType::BasicValue:
			AttributeValue.Value += ScaledEntryValue;
			break;
		case ELxEntryEffectiveType::BasicImprove:
		case ELxEntryEffectiveType::AdditionalImprove:
			AttributeValue.Value += AttributeValue.ValueLimit * ScaledEntryValue;
			break;
		case ELxEntryEffectiveType::Mechanism:
			AttributeValue.Value = FMath::Max(AttributeValue.Value, InEntryValue);
			break;
		}

		if (AttributeValue.ValueType == ELxCharacterValueType::RangedNumeric)
		{
			AttributeValue.ValueLimit = FMath::Max(0.0f, AttributeValue.ValueLimit);
			AttributeValue.Value = FMath::Clamp(AttributeValue.Value, 0.0f, AttributeValue.ValueLimit);
		}
	};

	switch (EntryBase->EntryType)
	{
	case ELxEntryType::AttributeGain:
		{
			const FLxEntryAttributeGain* GainEntry = static_cast<const FLxEntryAttributeGain*>(EntryBase);
			if (GainEntry->AttributeID != ELxCharacterAttributeID::X_None && GainEntry->AttributeID != InOutAttributeData.AttributeID)
			{
				return;
			}

			if (!GainEntry->TargetTags.IsEmpty() && !AttributeMatchesTargetTags(InOutAttributeData, GainEntry->TargetTags))
			{
				return;
			}

			ApplyValueToTarget(GainEntry->EntryTarget, GainEntry->EffectiveType, GainEntry->EntryValue);
		}
		break;
	case ELxEntryType::AttributeRecovery:
		{
			const FLxEntryAttributeRecovery* RecoveryEntry = static_cast<const FLxEntryAttributeRecovery*>(EntryBase);
			if (RecoveryEntry->AttributeID != ELxCharacterAttributeID::X_None && RecoveryEntry->AttributeID != InOutAttributeData.AttributeID)
			{
				return;
			}

			if (!RecoveryEntry->TargetTags.IsEmpty() && !AttributeMatchesTargetTags(InOutAttributeData, RecoveryEntry->TargetTags))
			{
				return;
			}

			const float RecoveryScale = InEntryObject.GetEntryQuote().EntryCD > KINDA_SMALL_NUMBER
				? InEntryObject.GetEntryQuote().EntryCD
				: 1.0f;
			ApplyRecoveryValue(RecoveryEntry->EffectiveType, RecoveryEntry->EntryValue, RecoveryScale);
		}
		break;
	default:
		break;
	}
}

void ULxCharacterAttributeComponent::ApplyDerivedRuleToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeDerivedRule& InDerivedRule, float InSourceValue)
{
	const float DerivedValue = InSourceValue * InDerivedRule.Ratio;
	const float DerivedRatio = InSourceValue * InDerivedRule.Ratio * 0.01f;
	FLxAttributeValue& ValueData = InOutAttributeData.CalculatedAttributeValue;

	auto ApplyDerivedValue = [DerivedValue, DerivedRatio](float& InOutTargetValue, ELxEntryEffectiveType InEntryType)
	{
		switch (InEntryType)
		{
		case ELxEntryEffectiveType::BasicValue:
			InOutTargetValue += DerivedValue;
			break;
		case ELxEntryEffectiveType::BasicImprove:
		case ELxEntryEffectiveType::AdditionalImprove:
			InOutTargetValue += InOutTargetValue * DerivedRatio;
			break;
		case ELxEntryEffectiveType::Mechanism:
			InOutTargetValue = FMath::Max(InOutTargetValue, DerivedValue);
			break;
		}
	};

	switch (InDerivedRule.EntryTarget)
	{
	case ELxEntryTarget::ToValueLimit:
		ApplyDerivedValue(ValueData.ValueLimit, InDerivedRule.EffectiveType);
		break;
	case ELxEntryTarget::ToValue:
		ApplyDerivedValue(ValueData.Value, InDerivedRule.EffectiveType);
		break;
	case ELxEntryTarget::ToUpwardFloatingRatio:
		ApplyDerivedValue(ValueData.UpwardFloatingRatio, InDerivedRule.EffectiveType);
		break;
	case ELxEntryTarget::ToDownwardFloatingRatio:
		ApplyDerivedValue(ValueData.DownwardFloatingRatio, InDerivedRule.EffectiveType);
		break;
	}
}

bool ULxCharacterAttributeComponent::AttributeMatchesTargetTags(const FLxAttributeData& InAttributeData, const FGameplayTagContainer& InTargetTags)
{
	if (InTargetTags.IsEmpty())
	{
		return false;
	}

	return InAttributeData.TargetTags.HasAll(InTargetTags);
}
