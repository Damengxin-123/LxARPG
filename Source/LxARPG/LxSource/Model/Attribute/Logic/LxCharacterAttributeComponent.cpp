#include "LxCharacterAttributeComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** 将属性架构使用的米换算为 Unreal 移动组件使用的厘米。 */
	constexpr float MetersToCentimeters = 100.f;

	/** 百分比属性换算为强度时，每1%对应的比例倍数。 */
	constexpr double PercentageUnitsPerRatio = 100.0;

	/** 将属性值四舍五入到最小细分单位后，再乘以换算指数得到整数强度。 */
	int64 ConvertAttributeUnitsToStrength(const double InAttributeUnits, const int32 InConversionIndex)
	{
		return FMath::RoundToInt64(InAttributeUnits) * static_cast<int64>(InConversionIndex);
	}

	/** 按词条修改方式更新指定数值。 */
	void ApplyOperation(float& InOutValue, const ELxAttributeModifierOperation InOperation, const float InModifierValue)
	{
		switch (InOperation)
		{
		case ELxAttributeModifierOperation::AddValue: InOutValue += InModifierValue; break;
		case ELxAttributeModifierOperation::AddBasePercent:
		case ELxAttributeModifierOperation::AddTotalPercent: InOutValue += InOutValue * InModifierValue * 0.01f; break;
		case ELxAttributeModifierOperation::UseMaximumValue: InOutValue = FMath::Max(InOutValue, InModifierValue); break;
		case ELxAttributeModifierOperation::UseMinimumValue: InOutValue = FMath::Min(InOutValue, InModifierValue); break;
		}
	}

	/** 按衍生规则方式更新指定数值。 */
	void ApplyDerivedOperation(float& InOutValue, const ELxEntryEffectiveType InOperation, const float InSourceValue, const float InRatio)
	{
		const float DerivedValue = InSourceValue * InRatio;
		switch (InOperation)
		{
		case ELxEntryEffectiveType::BasicValue: InOutValue += DerivedValue; break;
		case ELxEntryEffectiveType::BasicImprove:
		case ELxEntryEffectiveType::AdditionalImprove: InOutValue += InOutValue * DerivedValue * 0.01f; break;
		case ELxEntryEffectiveType::Mechanism: InOutValue = FMath::Max(InOutValue, DerivedValue); break;
		}
	}
}

ULxCharacterAttributeComponent::ULxCharacterAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULxCharacterAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterAttributeComponent, ReplicatedTypedAttributeSnapshot);
}

void ULxCharacterAttributeComponent::BaseComponentInitialize()
{
	InitializeRuntimeAttributeSet();
	InitializeAttributeTable();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::InitializeRuntimeAttributeSet()
{
	AttributeConfigurationTemplate = nullptr;
	RuntimeAttributeSet = nullptr;
	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	const TSubclassOf<ULxCharacterBaseAttributeSet> ConfigurationClass = OwnerCharacter != nullptr
		? OwnerCharacter->GetCharacterBaseAttributeSetClass()
		: nullptr;

	const TSubclassOf<ULxCharacterBaseAttributeSet> EffectiveConfigurationClass(
		ConfigurationClass != nullptr ? ConfigurationClass.Get() : ULxCharacterBaseAttributeSet::StaticClass());
	AttributeConfigurationTemplate = DuplicateObject<ULxCharacterBaseAttributeSet>(EffectiveConfigurationClass->GetDefaultObject<ULxCharacterBaseAttributeSet>(), this);

	ResetRuntimeAttributeSetFromConfiguration();
}

void ULxCharacterAttributeComponent::InitializeAttributeTable()
{
	AttributeModifierEffectCache.Reset();
	RuntimeResourceValues.Reset();
	ResetRuntimeAttributeSetFromConfiguration();
	RefreshDerivedAttributes();
	NormalizeTypedAttributeValues();
	CacheRuntimeResourceValues();
}

void ULxCharacterAttributeComponent::ResetRuntimeAttributeSetFromConfiguration()
{
	RuntimeAttributeSet = AttributeConfigurationTemplate != nullptr
		? DuplicateObject<ULxCharacterBaseAttributeSet>(AttributeConfigurationTemplate, this)
		: NewObject<ULxCharacterBaseAttributeSet>(this);
}

void ULxCharacterAttributeComponent::ReceiveAttributeModifierEffects(const FLxEffectSourceContext& InSourceContext, const ELxEffectPackageApplyPolicy InApplyPolicy, const TArray<FLxAttributeModifierEffect>& InEffectList)
{
	if (RuntimeAttributeSet == nullptr) InitializeRuntimeAttributeSet();
	if (InApplyPolicy == ELxEffectPackageApplyPolicy::Instant)
	{
		for (const FLxAttributeModifierEffect& Effect : InEffectList) ApplyModifierEffect(Effect);
		NormalizeTypedAttributeValues();
		CacheRuntimeResourceValues();
		BroadcastAttributeTableChanged();
		return;
	}

	FName SourceKey = InSourceContext.MakeSourceKey();
	if (SourceKey.IsNone()) SourceKey = TEXT("DefaultAttributeModifierEffectSource");
	if (InApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource) AttributeModifierEffectCache.Add(SourceKey, InEffectList);
	else AttributeModifierEffectCache.FindOrAdd(SourceKey).Append(InEffectList);
	RefreshCharacterAttributesByCachedEntries();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::ReceiveAttributeRecoveryEffects(const TArray<FLxAttributeRecoveryEffect>& InEffectList)
{
	if (RuntimeAttributeSet == nullptr) InitializeRuntimeAttributeSet();
	for (const FLxAttributeRecoveryEffect& Effect : InEffectList) ApplyRecoveryEffect(Effect);
	NormalizeTypedAttributeValues();
	CacheRuntimeResourceValues();
	BroadcastAttributeTableChanged();
}

void ULxCharacterAttributeComponent::RefreshCharacterAttributesByCachedEntries()
{
	CacheRuntimeResourceValues();
	ResetRuntimeAttributeSetFromConfiguration();
	for (const TPair<FName, TArray<FLxAttributeModifierEffect>>& EffectCachePair : AttributeModifierEffectCache)
	{
		for (const FLxAttributeModifierEffect& Effect : EffectCachePair.Value) ApplyModifierEffect(Effect);
	}
	RefreshDerivedAttributes();
	RestoreRuntimeResourceValues();
	NormalizeTypedAttributeValues();
	CacheRuntimeResourceValues();
}

void ULxCharacterAttributeComponent::ApplyModifierEffect(const FLxAttributeModifierEffect& InEffect)
{
	if (RuntimeAttributeSet == nullptr) return;
	TArray<FGameplayTag> TargetAttributeIDs;
	auto CollectTarget = [&InEffect, &TargetAttributeIDs](const auto& Attribute)
	{
		if (AttributeMatchesEffect(Attribute, InEffect.AttributeIDTag, InEffect.TargetAttributeCategories)) TargetAttributeIDs.AddUnique(Attribute.AttributeIDTag);
	};
	TArray<FLxBasicAttributeData> Basic; RuntimeAttributeSet->GetAllBasicAttributes(Basic); for (const auto& A : Basic) CollectTarget(A);
	TArray<FLxResourceAttributeData> Resource; RuntimeAttributeSet->GetAllResourceAttributes(Resource); for (const auto& A : Resource) CollectTarget(A);
	TArray<FLxProbabilityAttributeData> Probability; RuntimeAttributeSet->GetAllProbabilityAttributes(Probability); for (const auto& A : Probability) CollectTarget(A);
	TArray<FLxPercentageAttributeData> Percentage; RuntimeAttributeSet->GetAllPercentageAttributes(Percentage); for (const auto& A : Percentage) CollectTarget(A);
	TArray<FLxNumericAttributeData> Numeric; RuntimeAttributeSet->GetAllNumericAttributes(Numeric); for (const auto& A : Numeric) CollectTarget(A);
	TArray<FLxRangeAttributeData> Range; RuntimeAttributeSet->GetAllRangeAttributes(Range); for (const auto& A : Range) CollectTarget(A);

	for (const FGameplayTag AttributeIDTag : TargetAttributeIDs)
	{
		if (FLxBasicAttributeData* A = RuntimeAttributeSet->FindMutableBasicAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, InEffect.ModifierValue); continue; }
		if (FLxResourceAttributeData* A = RuntimeAttributeSet->FindMutableResourceAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, InEffect.ModifierValue); else if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValueLimit) ApplyOperation(A->ValueLimit, InEffect.ModifierOperation, InEffect.ModifierValue); continue; }
		if (FLxProbabilityAttributeData* A = RuntimeAttributeSet->FindMutableProbabilityAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, InEffect.ModifierValue); continue; }
		if (FLxPercentageAttributeData* A = RuntimeAttributeSet->FindMutablePercentageAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, InEffect.ModifierValue); continue; }
		if (FLxNumericAttributeData* A = RuntimeAttributeSet->FindMutableNumericAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, InEffect.ModifierValue); continue; }
		if (FLxRangeAttributeData* A = RuntimeAttributeSet->FindMutableRangeAttribute(AttributeIDTag))
		{
			if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, InEffect.ModifierValue);
			else if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToUpwardFloatingRatio) ApplyOperation(A->UpwardFloatingRatio, InEffect.ModifierOperation, InEffect.ModifierValue);
			else if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToDownwardFloatingRatio) ApplyOperation(A->DownwardFloatingRatio, InEffect.ModifierOperation, InEffect.ModifierValue);
		}
	}
}

void ULxCharacterAttributeComponent::ApplyRecoveryEffect(const FLxAttributeRecoveryEffect& InEffect)
{
	if (RuntimeAttributeSet == nullptr) return;
	TArray<FLxResourceAttributeData> Resources;
	RuntimeAttributeSet->GetAllResourceAttributes(Resources);
	for (const FLxResourceAttributeData& Resource : Resources)
	{
		if (!AttributeMatchesEffect(Resource, InEffect.AttributeIDTag, InEffect.TargetAttributeCategories)) continue;
		FLxResourceAttributeData* MutableResource = RuntimeAttributeSet->FindMutableResourceAttribute(Resource.AttributeIDTag);
		if (MutableResource == nullptr) continue;
		if (InEffect.RecoveryOperation == ELxAttributeModifierOperation::AddBasePercent || InEffect.RecoveryOperation == ELxAttributeModifierOperation::AddTotalPercent)
			MutableResource->Value += MutableResource->ValueLimit * InEffect.RecoveryValue * 0.01f;
		else ApplyOperation(MutableResource->Value, InEffect.RecoveryOperation, InEffect.RecoveryValue);
	}
}

void ULxCharacterAttributeComponent::RefreshDerivedAttributes()
{
	if (RuntimeAttributeSet == nullptr) return;
	TArray<FLxBasicAttributeData> BasicAttributes;
	RuntimeAttributeSet->GetAllBasicAttributes(BasicAttributes);
	for (const FLxBasicAttributeData& SourceAttribute : BasicAttributes)
	{
		for (const FLxAttributeDerivedRule& DerivedRule : SourceAttribute.DerivedRules)
		{
			const FGameplayTag TargetID = DerivedRule.AttributeIDTag;
			if (TargetID.IsValid() && TargetID != SourceAttribute.AttributeIDTag) ApplyDerivedValue(TargetID, DerivedRule, SourceAttribute.Value);
		}
	}
}

void ULxCharacterAttributeComponent::ApplyDerivedValue(const FGameplayTag InTargetAttributeIDTag, const FLxAttributeDerivedRule& InDerivedRule, const float InSourceValue)
{
	if (FLxBasicAttributeData* A = RuntimeAttributeSet->FindMutableBasicAttribute(InTargetAttributeIDTag)) { if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValue) ApplyDerivedOperation(A->Value, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio); return; }
	if (FLxResourceAttributeData* A = RuntimeAttributeSet->FindMutableResourceAttribute(InTargetAttributeIDTag)) { if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValue) ApplyDerivedOperation(A->Value, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio); else if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValueLimit) ApplyDerivedOperation(A->ValueLimit, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio); return; }
	if (FLxProbabilityAttributeData* A = RuntimeAttributeSet->FindMutableProbabilityAttribute(InTargetAttributeIDTag)) { if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValue) ApplyDerivedOperation(A->Value, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio); return; }
	if (FLxPercentageAttributeData* A = RuntimeAttributeSet->FindMutablePercentageAttribute(InTargetAttributeIDTag)) { if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValue) ApplyDerivedOperation(A->Value, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio); return; }
	if (FLxNumericAttributeData* A = RuntimeAttributeSet->FindMutableNumericAttribute(InTargetAttributeIDTag)) { if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValue) ApplyDerivedOperation(A->Value, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio); return; }
	if (FLxRangeAttributeData* A = RuntimeAttributeSet->FindMutableRangeAttribute(InTargetAttributeIDTag))
	{
		if (InDerivedRule.EntryTarget == ELxEntryTarget::ToValue) ApplyDerivedOperation(A->Value, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio);
		else if (InDerivedRule.EntryTarget == ELxEntryTarget::ToUpwardFloatingRatio) ApplyDerivedOperation(A->UpwardFloatingRatio, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio);
		else if (InDerivedRule.EntryTarget == ELxEntryTarget::ToDownwardFloatingRatio) ApplyDerivedOperation(A->DownwardFloatingRatio, InDerivedRule.EffectiveType, InSourceValue, InDerivedRule.Ratio);
	}
}

void ULxCharacterAttributeComponent::CacheRuntimeResourceValues()
{
	RuntimeResourceValues.Reset();
	if (RuntimeAttributeSet == nullptr) return;
	TArray<FLxResourceAttributeData> Resources; RuntimeAttributeSet->GetAllResourceAttributes(Resources);
	for (const FLxResourceAttributeData& Resource : Resources) RuntimeResourceValues.Add(Resource.AttributeIDTag, Resource.Value);
}

void ULxCharacterAttributeComponent::RestoreRuntimeResourceValues()
{
	if (RuntimeAttributeSet == nullptr) return;
	for (const TPair<FGameplayTag, float>& ResourcePair : RuntimeResourceValues)
	{
		if (FLxResourceAttributeData* Resource = RuntimeAttributeSet->FindMutableResourceAttribute(ResourcePair.Key)) Resource->Value = ResourcePair.Value;
	}
}

void ULxCharacterAttributeComponent::NormalizeTypedAttributeValues()
{
	if (RuntimeAttributeSet == nullptr) return;
	TArray<FLxResourceAttributeData> Resources; RuntimeAttributeSet->GetAllResourceAttributes(Resources);
	for (const FLxResourceAttributeData& Data : Resources) if (FLxResourceAttributeData* A = RuntimeAttributeSet->FindMutableResourceAttribute(Data.AttributeIDTag)) { A->ValueLimit = FMath::Max(0.f, FMath::RoundToFloat(A->ValueLimit)); A->Value = FMath::Clamp(FMath::RoundToFloat(A->Value), 0.f, A->ValueLimit); }
	TArray<FLxBasicAttributeData> Basic; RuntimeAttributeSet->GetAllBasicAttributes(Basic); for (const auto& Data : Basic) if (auto* A = RuntimeAttributeSet->FindMutableBasicAttribute(Data.AttributeIDTag)) A->Value = FMath::RoundToFloat(A->Value);
	TArray<FLxNumericAttributeData> Numeric; RuntimeAttributeSet->GetAllNumericAttributes(Numeric); for (const auto& Data : Numeric) if (auto* A = RuntimeAttributeSet->FindMutableNumericAttribute(Data.AttributeIDTag)) A->Value = FMath::RoundToFloat(A->Value);
	TArray<FLxProbabilityAttributeData> Probability; RuntimeAttributeSet->GetAllProbabilityAttributes(Probability); for (const auto& Data : Probability) if (auto* A = RuntimeAttributeSet->FindMutableProbabilityAttribute(Data.AttributeIDTag)) A->Value = FMath::Clamp(A->Value, 0.f, 1.f);
}

void ULxCharacterAttributeComponent::RefreshCharacterMovementSpeed() const
{
	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (OwnerCharacter == nullptr || RuntimeAttributeSet == nullptr) return;

	FLxNumericAttributeData BaseMovementSpeed;
	FLxPercentageAttributeData MovementSpeedBonus;
	if (!RuntimeAttributeSet->GetNumericAttribute(LxTag_Attribute_Numeric_BaseMovementSpeed, BaseMovementSpeed)
		|| !RuntimeAttributeSet->GetPercentageAttribute(LxTag_Attribute_Percentage_MovementSpeedBonus, MovementSpeedBonus))
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		const float MovementSpeedMetersPerSecond = FMath::Max(0.f, BaseMovementSpeed.Value * (1.f + MovementSpeedBonus.Value));
		MovementComponent->MaxWalkSpeed = MovementSpeedMetersPerSecond * MetersToCentimeters;
	}
}

bool ULxCharacterAttributeComponent::AttributeMatchesEffect(const FLxCharacterAttributeCommonData& InAttributeData, const FGameplayTag InAttributeIDTag, const TArray<ELxCharacterAttributeCategoryType>& InTargetCategories)
{
	const bool bHasID = InAttributeIDTag.IsValid();
	const bool bHasCategories = !InTargetCategories.IsEmpty();
	if (!bHasID && !bHasCategories) return false;
	if (bHasID && InAttributeData.AttributeIDTag != InAttributeIDTag) return false;
	return !bHasCategories || InTargetCategories.Contains(InAttributeData.AttributeCategory);
}

void ULxCharacterAttributeComponent::GetTypedAttributeSnapshot(FLxTypedAttributeSnapshot& OutAttributeSnapshot) const
{
	OutAttributeSnapshot = FLxTypedAttributeSnapshot();
	if (RuntimeAttributeSet == nullptr) return;
	RuntimeAttributeSet->GetAllBasicAttributes(OutAttributeSnapshot.BasicAttributes);
	RuntimeAttributeSet->GetAllResourceAttributes(OutAttributeSnapshot.ResourceAttributes);
	RuntimeAttributeSet->GetAllProbabilityAttributes(OutAttributeSnapshot.ProbabilityAttributes);
	RuntimeAttributeSet->GetAllPercentageAttributes(OutAttributeSnapshot.PercentageAttributes);
	RuntimeAttributeSet->GetAllNumericAttributes(OutAttributeSnapshot.NumericAttributes);
	RuntimeAttributeSet->GetAllRangeAttributes(OutAttributeSnapshot.RangeAttributes);
}

int32 ULxCharacterAttributeComponent::CalculateTotalStrength() const
{
	if (RuntimeAttributeSet == nullptr)
	{
		return 0;
	}

	FLxTypedAttributeSnapshot AttributeSnapshot;
	GetTypedAttributeSnapshot(AttributeSnapshot);

	int64 TotalStrength = 0;
	for (const FLxBasicAttributeData& AttributeData : AttributeSnapshot.BasicAttributes)
	{
		TotalStrength += ConvertAttributeUnitsToStrength(AttributeData.Value, AttributeData.StrengthConversionIndex);
	}
	for (const FLxResourceAttributeData& AttributeData : AttributeSnapshot.ResourceAttributes)
	{
		TotalStrength += ConvertAttributeUnitsToStrength(AttributeData.ValueLimit, AttributeData.StrengthConversionIndex);
	}
	for (const FLxProbabilityAttributeData& AttributeData : AttributeSnapshot.ProbabilityAttributes)
	{
		const double ProbabilityPercent = FMath::Clamp(static_cast<double>(AttributeData.Value) * PercentageUnitsPerRatio, 0.0, 100.0);
		TotalStrength += ConvertAttributeUnitsToStrength(ProbabilityPercent, AttributeData.StrengthConversionIndex);
	}
	for (const FLxPercentageAttributeData& AttributeData : AttributeSnapshot.PercentageAttributes)
	{
		const double PercentageValue = static_cast<double>(AttributeData.Value) * PercentageUnitsPerRatio;
		TotalStrength += ConvertAttributeUnitsToStrength(PercentageValue, AttributeData.StrengthConversionIndex);
	}
	for (const FLxNumericAttributeData& AttributeData : AttributeSnapshot.NumericAttributes)
	{
		TotalStrength += ConvertAttributeUnitsToStrength(AttributeData.Value, AttributeData.StrengthConversionIndex);
	}
	for (const FLxRangeAttributeData& AttributeData : AttributeSnapshot.RangeAttributes)
	{
		const double BaseValue = static_cast<double>(AttributeData.Value);
		const double LowerValue = BaseValue - BaseValue * static_cast<double>(AttributeData.DownwardFloatingRatio);
		const double UpperValue = BaseValue + BaseValue * static_cast<double>(AttributeData.UpwardFloatingRatio);
		const double AverageValue = (LowerValue + UpperValue) * 0.5;
		TotalStrength += ConvertAttributeUnitsToStrength(AverageValue, AttributeData.StrengthConversionIndex);
	}

	return static_cast<int32>(FMath::Clamp<int64>(TotalStrength, MIN_int32, MAX_int32));
}

void ULxCharacterAttributeComponent::BroadcastAttributeTableChanged()
{
	RefreshCharacterMovementSpeed();
	if (const AActor* OwnerActor = GetOwner(); OwnerActor != nullptr && OwnerActor->HasAuthority() && RuntimeAttributeSet != nullptr)
	{
		RuntimeAttributeSet->GetAllBasicAttributes(ReplicatedTypedAttributeSnapshot.BasicAttributes);
		RuntimeAttributeSet->GetAllResourceAttributes(ReplicatedTypedAttributeSnapshot.ResourceAttributes);
		RuntimeAttributeSet->GetAllProbabilityAttributes(ReplicatedTypedAttributeSnapshot.ProbabilityAttributes);
		RuntimeAttributeSet->GetAllPercentageAttributes(ReplicatedTypedAttributeSnapshot.PercentageAttributes);
		RuntimeAttributeSet->GetAllNumericAttributes(ReplicatedTypedAttributeSnapshot.NumericAttributes);
		RuntimeAttributeSet->GetAllRangeAttributes(ReplicatedTypedAttributeSnapshot.RangeAttributes);
	}
	FLxTypedAttributeSnapshot CurrentSnapshot;
	GetTypedAttributeSnapshot(CurrentSnapshot);
	OnTypedAttributeSnapshotChanged.Broadcast(CurrentSnapshot);
	OnDataChange.Broadcast();
}

void ULxCharacterAttributeComponent::OnRep_TypedAttributeSnapshot()
{
	if (RuntimeAttributeSet == nullptr) RuntimeAttributeSet = NewObject<ULxCharacterBaseAttributeSet>(this);
	RuntimeAttributeSet->ApplyTypedSnapshots(ReplicatedTypedAttributeSnapshot.BasicAttributes,
		ReplicatedTypedAttributeSnapshot.ResourceAttributes,
		ReplicatedTypedAttributeSnapshot.ProbabilityAttributes,
		ReplicatedTypedAttributeSnapshot.PercentageAttributes,
		ReplicatedTypedAttributeSnapshot.NumericAttributes,
		ReplicatedTypedAttributeSnapshot.RangeAttributes);
	RefreshCharacterMovementSpeed();
	OnTypedAttributeSnapshotChanged.Broadcast(ReplicatedTypedAttributeSnapshot);
	OnDataChange.Broadcast();
}
