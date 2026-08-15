#include "LxDefaultDamageCalculationNodes.h"

#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageTags.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

namespace
{
	float DefaultNodeGetAttributeSingleValue(ULxCharacterDataTransferComponent* DataTransferComponent, FGameplayTag AttributeIDTag, float DefaultValue = 0.f)
	{
		float AttributeValue = DefaultValue;
		return DataTransferComponent != nullptr && DataTransferComponent->QueryCharacterAttributeValue(AttributeIDTag, AttributeValue) ? AttributeValue : DefaultValue;
	}

	float DefaultNodeGetDamageSourceValue(ULxCharacterDataTransferComponent* DataTransferComponent, FGameplayTag AttributeIDTag)
	{
		FLxRangeAttributeData RangeAttribute;
		if (DataTransferComponent != nullptr && DataTransferComponent->QueryRangeAttribute(AttributeIDTag, RangeAttribute))
		{
			const float LowerValue = RangeAttribute.Value - RangeAttribute.Value * RangeAttribute.DownwardFloatingRatio;
			const float UpperValue = RangeAttribute.Value + RangeAttribute.Value * RangeAttribute.UpwardFloatingRatio;
			return FMath::FRandRange(FMath::Min(LowerValue, UpperValue), FMath::Max(LowerValue, UpperValue));
		}
		return DefaultNodeGetAttributeSingleValue(DataTransferComponent, AttributeIDTag);
	}

	void DefaultNodeNormalizeDamageEffect(FLxDamageEffect& InOutDamageEffect)
	{
		if (InOutDamageEffect.DamageValues.IsEmpty() && InOutDamageEffect.DamageValue > 0.f)
		{
			FLxDamageValue DamageValue;
			DamageValue.DamageTypeTag = LxTag_CommonEffect_DamageEffect_Normal;
			DamageValue.DamageValue = InOutDamageEffect.DamageValue;
			InOutDamageEffect.DamageValues.Add(DamageValue);
		}
	}

	float DefaultNodeGetTotalDamageValue(const TArray<FLxDamageEffect>& DamageEffects)
	{
		float TotalDamageValue = 0.f;
		for (const FLxDamageEffect& DamageEffect : DamageEffects)
		{
			for (const FLxDamageValue& DamageValue : DamageEffect.DamageValues)
			{
				TotalDamageValue += FMath::Max(0.f, DamageValue.DamageValue);
			}
		}

		return TotalDamageValue;
	}
}

ULxDamageAttackPowerOutputNode::ULxDamageAttackPowerOutputNode()
{
	DamageTypeTag = LxTag_CommonEffect_DamageEffect_Normal;
}

void ULxDamageAttackPowerOutputNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	const float RolledDamageValue = FMath::Max(0.f, DefaultNodeGetDamageSourceValue(InOutContext.SourceDataTransferComponent, LxTag_Attribute_Combat_AttackPower));
	if (FMath::IsNearlyZero(RolledDamageValue))
	{
		return;
	}

	FLxDamageValue DamageValue;
	DamageValue.DamageTypeTag = DamageTypeTag.IsValid() ? DamageTypeTag : LxTag_CommonEffect_DamageEffect_Normal;
	DamageValue.DamageValue = RolledDamageValue;

	FLxDamageEffect DamageEffect;
	DamageEffect.TargetAttributeIDTag = LxTag_Attribute_Resource_Health;
	DamageEffect.DamageValue = RolledDamageValue;
	DamageEffect.DamageValues.Add(DamageValue);
	InOutContext.DamageEffects.Add(DamageEffect);
}

void ULxDamageCriticalOutputNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	if (InOutContext.DamageEffects.IsEmpty())
	{
		return;
	}

	const float CriticalChance = FMath::Clamp(DefaultNodeGetAttributeSingleValue(InOutContext.SourceDataTransferComponent, LxTag_Attribute_Judgement_CriticalChance), 0.f, 1.f);
	if (FMath::FRand() > CriticalChance)
	{
		return;
	}

	float CriticalDamageMultiplier = DefaultCriticalDamageMultiplier;
	const float CriticalDamagePercent = DefaultNodeGetAttributeSingleValue(InOutContext.SourceDataTransferComponent, LxTag_Attribute_Combat_CriticalDamage, 0.f);
	if (CriticalDamagePercent > 0.f)
	{
		CriticalDamageMultiplier = 1.f + CriticalDamagePercent;
	}

	CriticalDamageMultiplier = FMath::Max(1.f, CriticalDamageMultiplier);
	InOutContext.bCriticalHit = true;
	for (FLxDamageEffect& DamageEffect : InOutContext.DamageEffects)
	{
		DefaultNodeNormalizeDamageEffect(DamageEffect);
		DamageEffect.bCriticalHit = true;
		DamageEffect.DamageValue *= CriticalDamageMultiplier;
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			DamageValue.DamageValue *= CriticalDamageMultiplier;
		}
	}
}

void ULxDamageDefenseReceiveNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	const float DefenseValue = FMath::Max(0.f, DefaultNodeGetAttributeSingleValue(InOutContext.TargetDataTransferComponent, LxTag_Attribute_Combat_Armor));
	InOutContext.DefenseValue = DefenseValue;
	if (FMath::IsNearlyZero(DefenseValue))
	{
		return;
	}

	for (FLxDamageEffect& DamageEffect : InOutContext.DamageEffects)
	{
		DefaultNodeNormalizeDamageEffect(DamageEffect);
		float TotalReducedDamageValue = 0.f;
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			const float SourceDamageValue = FMath::Max(0.f, DamageValue.DamageValue);
			DamageValue.DamageValue = SourceDamageValue > 0.f ? SourceDamageValue * SourceDamageValue / (SourceDamageValue + DefenseValue) : 0.f;
			TotalReducedDamageValue += DamageValue.DamageValue;
		}
		DamageEffect.DamageValue = TotalReducedDamageValue;
	}
}

void ULxDamageShieldHealthSettlementNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	const float TotalDamageValue = DefaultNodeGetTotalDamageValue(InOutContext.DamageEffects);
	if (FMath::IsNearlyZero(TotalDamageValue))
	{
		return;
	}

	const float ShieldValue = FMath::Max(0.f, DefaultNodeGetAttributeSingleValue(InOutContext.TargetDataTransferComponent, LxTag_Attribute_Resource_Shield));
	InOutContext.ShieldDamageValue = FMath::Min(ShieldValue, TotalDamageValue);
	InOutContext.HealthDamageValue = FMath::Max(0.f, TotalDamageValue - InOutContext.ShieldDamageValue);
}
