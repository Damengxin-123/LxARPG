#include "LxCharacterAttributeComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxCharacterBaseAttributeConfig.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** 将属性架构使用的米换算为 Unreal 移动组件使用的厘米。 */
	constexpr float AttributeMetersToCentimeters = 100.f;

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
	AttributeConfigurationTemplate = NewObject<ULxCharacterBaseAttributeSet>(this);

	FLxCharacterBaseAttributeConfig AttributeConfig;
	if (OwnerCharacter != nullptr
		&& ULxCharacterBaseAttributeConfigFunctionLibrary::GetCharacterBaseAttributeConfig(
			OwnerCharacter->GetCharacterIDTag(), AttributeConfig))
	{
		AttributeConfigurationTemplate->ApplyBaseAttributeConfig(AttributeConfig);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("角色属性初始化未找到配置，角色=%s，角色ID=%s，将使用原生默认属性。"),
			*GetNameSafe(OwnerCharacter),
			OwnerCharacter != nullptr ? *OwnerCharacter->GetCharacterIDTag().ToString() : TEXT("无效角色"));
	}

	ResetRuntimeAttributeSetFromConfiguration();
}

void ULxCharacterAttributeComponent::InitializeAttributeTable()
{
	AttributeModifierEffectCache.Reset();
	RuntimeResourceValues.Reset();
	ResetRuntimeAttributeSetFromConfiguration();
	NormalizeTypedAttributeValues();
	FillRuntimeResourceValuesToLimit();
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
		for (const FLxAttributeModifierEffect& Effect : InEffectList)
		{
			if (!Effect.SourceAttributeIDTag.IsValid()) ApplyModifierEffect(Effect);
		}
		for (const FLxAttributeModifierEffect& Effect : InEffectList)
		{
			if (Effect.SourceAttributeIDTag.IsValid()) ApplyModifierEffect(Effect);
		}
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
		for (const FLxAttributeModifierEffect& Effect : EffectCachePair.Value)
		{
			if (!Effect.SourceAttributeIDTag.IsValid()) ApplyModifierEffect(Effect);
		}
	}
	for (const TPair<FName, TArray<FLxAttributeModifierEffect>>& EffectCachePair : AttributeModifierEffectCache)
	{
		for (const FLxAttributeModifierEffect& Effect : EffectCachePair.Value)
		{
			if (Effect.SourceAttributeIDTag.IsValid()) ApplyModifierEffect(Effect);
		}
	}
	RestoreRuntimeResourceValues();
	NormalizeTypedAttributeValues();
	CacheRuntimeResourceValues();
}

void ULxCharacterAttributeComponent::ApplyModifierEffect(const FLxAttributeModifierEffect& InEffect)
{
	if (RuntimeAttributeSet == nullptr) return;
	float EffectiveModifierValue = InEffect.ModifierValue;
	if (InEffect.SourceAttributeIDTag.IsValid())
	{
		float SourceAttributeValue = 0.f;
		if (InEffect.SourceAttributeIDTag == InEffect.AttributeIDTag
			|| !TryGetAttributeFieldValue(InEffect.SourceAttributeIDTag, InEffect.SourceAttributeTarget, SourceAttributeValue))
		{
			return;
		}
		EffectiveModifierValue += SourceAttributeValue * InEffect.SourceAttributeRatio;
	}

	TArray<FGameplayTag> TargetAttributeIDs;
	auto CollectTarget = [&InEffect, &TargetAttributeIDs](const auto& Attribute)
	{
		if (AttributeMatchesEffect(Attribute, InEffect.AttributeIDTag, InEffect.TargetBusinessCategories))
		{
			TargetAttributeIDs.AddUnique(Attribute.AttributeIDTag);
		}
	};
	TArray<FLxScalarAttributeData> Scalar; RuntimeAttributeSet->GetAllScalarAttributes(Scalar); for (const auto& A : Scalar) CollectTarget(A);
	TArray<FLxResourceAttributeData> Resource; RuntimeAttributeSet->GetAllResourceAttributes(Resource); for (const auto& A : Resource) CollectTarget(A);
	TArray<FLxRangeAttributeData> Range; RuntimeAttributeSet->GetAllRangeAttributes(Range); for (const auto& A : Range) CollectTarget(A);

	for (const FGameplayTag AttributeIDTag : TargetAttributeIDs)
	{
		if (FLxScalarAttributeData* A = RuntimeAttributeSet->FindMutableScalarAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, EffectiveModifierValue); continue; }
		if (FLxResourceAttributeData* A = RuntimeAttributeSet->FindMutableResourceAttribute(AttributeIDTag)) { if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, EffectiveModifierValue); else if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValueLimit) ApplyOperation(A->ValueLimit, InEffect.ModifierOperation, EffectiveModifierValue); continue; }
		if (FLxRangeAttributeData* A = RuntimeAttributeSet->FindMutableRangeAttribute(AttributeIDTag))
		{
			if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToValue) ApplyOperation(A->Value, InEffect.ModifierOperation, EffectiveModifierValue);
			else if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToUpwardFloatingRatio) ApplyOperation(A->UpwardFloatingRatio, InEffect.ModifierOperation, EffectiveModifierValue);
			else if (InEffect.ModifierTarget == ELxAttributeModifierTarget::ToDownwardFloatingRatio) ApplyOperation(A->DownwardFloatingRatio, InEffect.ModifierOperation, EffectiveModifierValue);
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
		if (!AttributeMatchesEffect(Resource, InEffect.AttributeIDTag, InEffect.TargetBusinessCategories)) continue;
		FLxResourceAttributeData* MutableResource = RuntimeAttributeSet->FindMutableResourceAttribute(Resource.AttributeIDTag);
		if (MutableResource == nullptr) continue;
		if (InEffect.RecoveryOperation == ELxAttributeModifierOperation::AddBasePercent || InEffect.RecoveryOperation == ELxAttributeModifierOperation::AddTotalPercent)
			MutableResource->Value += MutableResource->ValueLimit * InEffect.RecoveryValue * 0.01f;
		else ApplyOperation(MutableResource->Value, InEffect.RecoveryOperation, InEffect.RecoveryValue);
	}
}

bool ULxCharacterAttributeComponent::TryGetAttributeFieldValue(const FGameplayTag InAttributeIDTag,
	const ELxAttributeModifierTarget InAttributeTarget, float& OutAttributeValue) const
{
	OutAttributeValue = 0.f;
	if (RuntimeAttributeSet == nullptr || !InAttributeIDTag.IsValid()) return false;

	FLxScalarAttributeData ScalarAttribute;
	if (RuntimeAttributeSet->GetScalarAttribute(InAttributeIDTag, ScalarAttribute))
	{
		if (InAttributeTarget != ELxAttributeModifierTarget::ToValue) return false;
		OutAttributeValue = ScalarAttribute.Value;
		return true;
	}

	FLxResourceAttributeData ResourceAttribute;
	if (RuntimeAttributeSet->GetResourceAttribute(InAttributeIDTag, ResourceAttribute))
	{
		if (InAttributeTarget == ELxAttributeModifierTarget::ToValue) OutAttributeValue = ResourceAttribute.Value;
		else if (InAttributeTarget == ELxAttributeModifierTarget::ToValueLimit) OutAttributeValue = ResourceAttribute.ValueLimit;
		else return false;
		return true;
	}

	FLxRangeAttributeData RangeAttribute;
	if (RuntimeAttributeSet->GetRangeAttribute(InAttributeIDTag, RangeAttribute))
	{
		if (InAttributeTarget == ELxAttributeModifierTarget::ToValue) OutAttributeValue = RangeAttribute.Value;
		else if (InAttributeTarget == ELxAttributeModifierTarget::ToUpwardFloatingRatio) OutAttributeValue = RangeAttribute.UpwardFloatingRatio;
		else if (InAttributeTarget == ELxAttributeModifierTarget::ToDownwardFloatingRatio) OutAttributeValue = RangeAttribute.DownwardFloatingRatio;
		else return false;
		return true;
	}

	return false;
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

void ULxCharacterAttributeComponent::FillRuntimeResourceValuesToLimit()
{
	if (RuntimeAttributeSet == nullptr) return;

	TArray<FLxResourceAttributeData> Resources;
	RuntimeAttributeSet->GetAllResourceAttributes(Resources);
	for (const FLxResourceAttributeData& ResourceData : Resources)
	{
		if (FLxResourceAttributeData* Resource = RuntimeAttributeSet->FindMutableResourceAttribute(ResourceData.AttributeIDTag))
		{
			Resource->Value = Resource->ValueLimit;
		}
	}
}

void ULxCharacterAttributeComponent::NormalizeTypedAttributeValues()
{
	if (RuntimeAttributeSet == nullptr) return;
	TArray<FLxResourceAttributeData> Resources; RuntimeAttributeSet->GetAllResourceAttributes(Resources);
	for (const FLxResourceAttributeData& Data : Resources) if (FLxResourceAttributeData* A = RuntimeAttributeSet->FindMutableResourceAttribute(Data.AttributeIDTag)) { A->ValueLimit = FMath::Max(0.f, FMath::RoundToFloat(A->ValueLimit)); A->Value = FMath::Clamp(FMath::RoundToFloat(A->Value), 0.f, A->ValueLimit); }
	TArray<FLxScalarAttributeData> Scalar; RuntimeAttributeSet->GetAllScalarAttributes(Scalar); for (const auto& Data : Scalar) if (auto* A = RuntimeAttributeSet->FindMutableScalarAttribute(Data.AttributeIDTag)) A->Value = A->ScalarRule.NormalizeValue(A->Value);
}

void ULxCharacterAttributeComponent::RefreshCharacterMovementSpeed() const
{
	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (OwnerCharacter == nullptr || RuntimeAttributeSet == nullptr) return;

	FLxScalarAttributeData BaseMovementSpeed;
	FLxScalarAttributeData MovementSpeedBonus;
	if (!RuntimeAttributeSet->GetScalarAttribute(LxTag_Attribute_Action_BaseMovementSpeed, BaseMovementSpeed)
		|| !RuntimeAttributeSet->GetScalarAttribute(LxTag_Attribute_Action_MovementSpeedBonus, MovementSpeedBonus))
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		const float MovementSpeedMetersPerSecond = FMath::Max(0.f, BaseMovementSpeed.Value * (1.f + MovementSpeedBonus.Value));
		MovementComponent->MaxWalkSpeed = MovementSpeedMetersPerSecond * AttributeMetersToCentimeters;
	}
}

bool ULxCharacterAttributeComponent::AttributeMatchesEffect(const FLxCharacterAttributeCommonData& InAttributeData,
	const FGameplayTag InAttributeIDTag, const TArray<ELxCharacterAttributeBusinessCategory>& InTargetBusinessCategories)
{
	const bool bHasID = InAttributeIDTag.IsValid();
	const bool bHasBusinessCategories = !InTargetBusinessCategories.IsEmpty();
	if (!bHasID && !bHasBusinessCategories) return false;
	if (bHasID && InAttributeData.AttributeIDTag != InAttributeIDTag) return false;
	return !bHasBusinessCategories || InTargetBusinessCategories.Contains(InAttributeData.BusinessCategory);
}

void ULxCharacterAttributeComponent::GetTypedAttributeSnapshot(FLxTypedAttributeSnapshot& OutAttributeSnapshot) const
{
	OutAttributeSnapshot = FLxTypedAttributeSnapshot();
	if (RuntimeAttributeSet == nullptr) return;
	RuntimeAttributeSet->GetAllScalarAttributes(OutAttributeSnapshot.ScalarAttributes);
	RuntimeAttributeSet->GetAllResourceAttributes(OutAttributeSnapshot.ResourceAttributes);
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
	for (const FLxScalarAttributeData& AttributeData : AttributeSnapshot.ScalarAttributes)
	{
		const double AttributeUnits = AttributeData.ScalarRule.DisplayFormat == ELxScalarAttributeDisplayFormat::Percentage
			? static_cast<double>(AttributeData.Value) * PercentageUnitsPerRatio
			: static_cast<double>(AttributeData.Value);
		TotalStrength += ConvertAttributeUnitsToStrength(AttributeUnits, AttributeData.StrengthConversionIndex);
	}
	for (const FLxResourceAttributeData& AttributeData : AttributeSnapshot.ResourceAttributes)
	{
		TotalStrength += ConvertAttributeUnitsToStrength(AttributeData.ValueLimit, AttributeData.StrengthConversionIndex);
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
		RuntimeAttributeSet->GetAllScalarAttributes(ReplicatedTypedAttributeSnapshot.ScalarAttributes);
		RuntimeAttributeSet->GetAllResourceAttributes(ReplicatedTypedAttributeSnapshot.ResourceAttributes);
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
	RuntimeAttributeSet->ApplyTypedSnapshots(ReplicatedTypedAttributeSnapshot.ScalarAttributes,
		ReplicatedTypedAttributeSnapshot.ResourceAttributes,
		ReplicatedTypedAttributeSnapshot.RangeAttributes);
	RefreshCharacterMovementSpeed();
	OnTypedAttributeSnapshotChanged.Broadcast(ReplicatedTypedAttributeSnapshot);
	OnDataChange.Broadcast();
}
