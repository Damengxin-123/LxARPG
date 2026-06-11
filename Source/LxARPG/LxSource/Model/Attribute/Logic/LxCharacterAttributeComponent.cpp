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

	FGameplayTag ResolveAttributeIDTag(const FLxAttributeValueConfig& InValueConfig)
	{
		return LxAttributeTools::ResolveAttributeIDTag(InValueConfig);
	}

	void ApplyBaseValueConfigToAttribute(const FLxAttributeValueConfig& InValueConfig, FLxAttributeData& InOutAttributeData)
	{
		if (InValueConfig.AttributeIDTag.IsValid())
		{
			InOutAttributeData.AttributeIDTag = InValueConfig.AttributeIDTag;
		}
		else if (!InOutAttributeData.AttributeIDTag.IsValid())
		{
			InOutAttributeData.AttributeIDTag = LxAttributeTools::GetAttributeIDTagByLegacyID(InValueConfig.AttributeID);
		}

		InOutAttributeData.AttributeValue.UpwardFloatingRatio = InValueConfig.UpwardFloatingRatio;
		InOutAttributeData.AttributeValue.DownwardFloatingRatio = InValueConfig.DownwardFloatingRatio;
		InOutAttributeData.AttributeValue.ValueLimit = InValueConfig.ValueLimit;
		InOutAttributeData.AttributeValue.Value = InValueConfig.Value;
		InOutAttributeData.CalculatedAttributeValue = InOutAttributeData.AttributeValue;
	}

	void ApplyBaseValueConfigsToAttributeMap(const TArray<FLxAttributeValueConfig>& InValueConfigs, TMap<FGameplayTag, FLxAttributeData>& InOutAttributeDataMap)
	{
		for (const FLxAttributeValueConfig& ValueConfig : InValueConfigs)
		{
			const FGameplayTag AttributeIDTag = ResolveAttributeIDTag(ValueConfig);
			if (!AttributeIDTag.IsValid())
			{
				continue;
			}

			FLxAttributeData* FoundAttributeData = InOutAttributeDataMap.Find(AttributeIDTag);
			if (FoundAttributeData == nullptr)
			{
				FLxAttributeData NewData;
				ApplyBaseValueConfigToAttribute(ValueConfig, NewData);
				if (!NewData.AttributeIDTag.IsValid())
				{
					NewData.AttributeIDTag = AttributeIDTag;
				}
				InOutAttributeDataMap.Add(AttributeIDTag, NewData);
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

		for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
		{
			ApplyEntryToAttribute(AttributePair.Value, *EntryObject);
		}
	}

	for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}

	CacheRuntimeRangedAttributeValues();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::ReceiveAttributeModifierEffects(const FLxEffectSourceContext& InSourceContext, ELxEffectPackageApplyPolicy InApplyPolicy, const TArray<FLxAttributeModifierEffect>& InEffectList)
{
	if (CharacterAttributeTable.IsEmpty())
	{
		InitializeAttributeTable();
	}

	if (InApplyPolicy == ELxEffectPackageApplyPolicy::Instant)
	{
		for (const FLxAttributeModifierEffect& Effect : InEffectList)
		{
			for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
			{
				ApplyModifierEffectToAttribute(AttributePair.Value, Effect);
			}
		}

		for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
		{
			NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
		}

		CacheRuntimeRangedAttributeValues();
		BroadcastAttributeTableChanged();
		return;
	}

	FName SourceKey = InSourceContext.MakeSourceKey();
	if (SourceKey.IsNone())
	{
		SourceKey = TEXT("DefaultAttributeModifierEffectSource");
	}

	if (InApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource)
	{
		AttributeModifierEffectCache.Add(SourceKey, InEffectList);
	}
	else
	{
		AttributeModifierEffectCache.FindOrAdd(SourceKey).Append(InEffectList);
	}

	RefreshCharacterAttributesByCachedEntries();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::ReceiveAttributeRecoveryEffects(const TArray<FLxAttributeRecoveryEffect>& InEffectList)
{
	if (CharacterAttributeTable.IsEmpty())
	{
		InitializeAttributeTable();
	}

	for (const FLxAttributeRecoveryEffect& Effect : InEffectList)
	{
		for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
		{
			ApplyRecoveryEffectToAttribute(AttributePair.Value, Effect);
		}
	}

	for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}

	CacheRuntimeRangedAttributeValues();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::GetCharacterAttributeList(TArray<FLxAttributeData>& OutAttributeList) const
{
	OutAttributeList.Reset();
	for (const TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		OutAttributeList.Add(AttributePair.Value);
	}
}

const FLxAttributeData* ULxCharacterAttributeComponent::GetCharacterAttributeByIDTag(FGameplayTag InAttributeIDTag) const
{
	return CharacterAttributeTable.Find(InAttributeIDTag);
}

void ULxCharacterAttributeComponent::InitializeAttributeTable()
{
	AttributeGainEntryCache.Empty();
	AttributeModifierEffectCache.Empty();
	RuntimeRangedAttributeValues.Empty();
	RebuildAttributeTableFromRaceConfig();
	RefreshDerivedAttributes();
	for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
	CacheRuntimeRangedAttributeValues();
}

void ULxCharacterAttributeComponent::RebuildAttributeTableFromRaceConfig()
{
	CharacterAttributeTable.Empty();

	TMap<FGameplayTag, FLxAttributeData> AttributeDataMap = CharacterAttributeValueConfigs.IsEmpty()
		? LxAttributeConfig::GetCharacterAttributeDataByRaceType(CharacterRaceType)
		: LxAttributeConfig::GetAttributeDataMap();
	if (!CharacterAttributeValueConfigs.IsEmpty())
	{
		ApplyBaseValueConfigsToAttributeMap(CharacterAttributeValueConfigs, AttributeDataMap);
	}

	for (const TPair<FGameplayTag, FLxAttributeData>& AttributePair : AttributeDataMap)
	{
		FLxAttributeData AttributeData = AttributePair.Value;
		if (!LxAttributeTools::NormalizeAttributeIDTag(AttributeData))
		{
			continue;
		}

		CharacterAttributeTable.Add(AttributeData.AttributeIDTag, AttributeData);
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

			for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
			{
				ApplyEntryToAttribute(AttributePair.Value, *EntryObject);
			}
		}
	}

	for (const TPair<FName, TArray<FLxAttributeModifierEffect>>& EffectCachePair : AttributeModifierEffectCache)
	{
		for (const FLxAttributeModifierEffect& Effect : EffectCachePair.Value)
		{
			for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
			{
				ApplyModifierEffectToAttribute(AttributePair.Value, Effect);
			}
		}
	}

	RefreshDerivedAttributes();
	RestoreRuntimeRangedAttributeValues();

	for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		NormalizeAttributeValueByType(AttributePair.Value.CalculatedAttributeValue);
	}
	CacheRuntimeRangedAttributeValues();
}

void ULxCharacterAttributeComponent::CacheRuntimeRangedAttributeValues()
{
	RuntimeRangedAttributeValues.Reset();
	for (const TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
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
	for (TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
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
	TMap<FGameplayTag, float> SourceValueSnapshot;
	SourceValueSnapshot.Reserve(CharacterAttributeTable.Num());

	for (const TPair<FGameplayTag, FLxAttributeData>& AttributePair : CharacterAttributeTable)
	{
		SourceValueSnapshot.Add(AttributePair.Key, AttributePair.Value.CalculatedAttributeValue.Value);
	}

	for (const TPair<FGameplayTag, FLxAttributeData>& SourcePair : CharacterAttributeTable)
	{
		const float* SourceValue = SourceValueSnapshot.Find(SourcePair.Key);
		if (SourceValue == nullptr || FMath::IsNearlyZero(*SourceValue))
		{
			continue;
		}

		for (const FLxAttributeDerivedRule& DerivedRule : SourcePair.Value.DerivedRulesArray)
		{
			const FGameplayTag TargetAttributeIDTag = LxAttributeTools::ResolveAttributeIDTag(DerivedRule);
			if (!TargetAttributeIDTag.IsValid() || FMath::IsNearlyZero(DerivedRule.Ratio))
			{
				continue;
			}

			if (TargetAttributeIDTag == SourcePair.Key)
			{
				continue;
			}

			if (FLxAttributeData* TargetAttributeData = CharacterAttributeTable.Find(TargetAttributeIDTag))
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
			const FGameplayTag EntryAttributeIDTag = GainEntry->AttributeIDTag.IsValid()
				? GainEntry->AttributeIDTag
				: LxAttributeTools::GetAttributeIDTagByLegacyID(GainEntry->AttributeID);
			if (EntryAttributeIDTag.IsValid() && EntryAttributeIDTag != InOutAttributeData.AttributeIDTag)
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
			const FGameplayTag EntryAttributeIDTag = RecoveryEntry->AttributeIDTag.IsValid()
				? RecoveryEntry->AttributeIDTag
				: LxAttributeTools::GetAttributeIDTagByLegacyID(RecoveryEntry->AttributeID);
			if (EntryAttributeIDTag.IsValid() && EntryAttributeIDTag != InOutAttributeData.AttributeIDTag)
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

void ULxCharacterAttributeComponent::ApplyModifierEffectToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeModifierEffect& InEffect)
{
	if (!AttributeMatchesModifierEffect(InOutAttributeData, InEffect))
	{
		return;
	}

	auto ApplyModifierValue = [&InEffect](float& InOutTargetValue)
	{
		switch (InEffect.ModifierOperation)
		{
		case ELxAttributeModifierOperation::AddValue:
			InOutTargetValue += InEffect.ModifierValue;
			break;
		case ELxAttributeModifierOperation::AddBasePercent:
		case ELxAttributeModifierOperation::AddTotalPercent:
			InOutTargetValue += InOutTargetValue * InEffect.ModifierValue * 0.01f;
			break;
		case ELxAttributeModifierOperation::UseMaximumValue:
			InOutTargetValue = FMath::Max(InOutTargetValue, InEffect.ModifierValue);
			break;
		case ELxAttributeModifierOperation::UseMinimumValue:
			InOutTargetValue = FMath::Min(InOutTargetValue, InEffect.ModifierValue);
			break;
		}
	};

	FLxAttributeValue& AttributeValue = InOutAttributeData.CalculatedAttributeValue;
	switch (InEffect.ModifierTarget)
	{
	case ELxAttributeModifierTarget::ToValue:
		ApplyModifierValue(AttributeValue.Value);
		break;
	case ELxAttributeModifierTarget::ToValueLimit:
		ApplyModifierValue(AttributeValue.ValueLimit);
		break;
	case ELxAttributeModifierTarget::ToUpwardFloatingRatio:
		ApplyModifierValue(AttributeValue.UpwardFloatingRatio);
		break;
	case ELxAttributeModifierTarget::ToDownwardFloatingRatio:
		ApplyModifierValue(AttributeValue.DownwardFloatingRatio);
		break;
	}
}

void ULxCharacterAttributeComponent::ApplyRecoveryEffectToAttribute(FLxAttributeData& InOutAttributeData, const FLxAttributeRecoveryEffect& InEffect)
{
	if (!AttributeMatchesRecoveryEffect(InOutAttributeData, InEffect))
	{
		return;
	}

	FLxAttributeValue& AttributeValue = InOutAttributeData.CalculatedAttributeValue;
	switch (InEffect.RecoveryOperation)
	{
	case ELxAttributeModifierOperation::AddValue:
		AttributeValue.Value += InEffect.RecoveryValue;
		break;
	case ELxAttributeModifierOperation::AddBasePercent:
	case ELxAttributeModifierOperation::AddTotalPercent:
		AttributeValue.Value += AttributeValue.ValueLimit * InEffect.RecoveryValue * 0.01f;
		break;
	case ELxAttributeModifierOperation::UseMaximumValue:
		AttributeValue.Value = FMath::Max(AttributeValue.Value, InEffect.RecoveryValue);
		break;
	case ELxAttributeModifierOperation::UseMinimumValue:
		AttributeValue.Value = FMath::Min(AttributeValue.Value, InEffect.RecoveryValue);
		break;
	}

	if (AttributeValue.ValueType == ELxCharacterValueType::RangedNumeric)
	{
		AttributeValue.ValueLimit = FMath::Max(0.0f, AttributeValue.ValueLimit);
		AttributeValue.Value = FMath::Clamp(AttributeValue.Value, 0.0f, AttributeValue.ValueLimit);
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

bool ULxCharacterAttributeComponent::AttributeMatchesModifierEffect(const FLxAttributeData& InAttributeData, const FLxAttributeModifierEffect& InEffect)
{
	const FGameplayTag EffectAttributeIDTag = InEffect.AttributeIDTag.IsValid()
		? InEffect.AttributeIDTag
		: LxAttributeTools::GetAttributeIDTagByLegacyID(InEffect.AttributeID);
	const bool bHasAttributeIDTag = EffectAttributeIDTag.IsValid();
	const bool bHasTargetTags = !InEffect.TargetTags.IsEmpty();
	if (!bHasAttributeIDTag && !bHasTargetTags)
	{
		return false;
	}

	if (bHasAttributeIDTag && EffectAttributeIDTag != InAttributeData.AttributeIDTag)
	{
		return false;
	}

	return !bHasTargetTags || AttributeMatchesTargetTags(InAttributeData, InEffect.TargetTags);
}

bool ULxCharacterAttributeComponent::AttributeMatchesRecoveryEffect(const FLxAttributeData& InAttributeData, const FLxAttributeRecoveryEffect& InEffect)
{
	const FGameplayTag EffectAttributeIDTag = InEffect.AttributeIDTag.IsValid()
		? InEffect.AttributeIDTag
		: LxAttributeTools::GetAttributeIDTagByLegacyID(InEffect.AttributeID);
	const bool bHasAttributeIDTag = EffectAttributeIDTag.IsValid();
	const bool bHasTargetTags = !InEffect.TargetTags.IsEmpty();
	if (!bHasAttributeIDTag && !bHasTargetTags)
	{
		return false;
	}

	if (bHasAttributeIDTag && EffectAttributeIDTag != InAttributeData.AttributeIDTag)
	{
		return false;
	}

	return !bHasTargetTags || AttributeMatchesTargetTags(InAttributeData, InEffect.TargetTags);
}
