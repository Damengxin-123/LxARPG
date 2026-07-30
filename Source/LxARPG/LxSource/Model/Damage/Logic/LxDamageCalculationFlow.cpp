#include "LxDamageCalculationFlow.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageTags.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

namespace
{
	float GetAttributeSingleValue(ULxCharacterDataTransferComponent* DataTransferComponent, FGameplayTag AttributeIDTag, float DefaultValue = 0.f)
	{
		float AttributeValue = DefaultValue;
		return DataTransferComponent != nullptr && DataTransferComponent->QueryCharacterAttributeValue(AttributeIDTag, AttributeValue) ? AttributeValue : DefaultValue;
	}

	float GetDamageSourceValue(ULxCharacterDataTransferComponent* DataTransferComponent, FGameplayTag AttributeIDTag)
	{
		FLxRangeAttributeData RangeAttribute;
		if (DataTransferComponent != nullptr && DataTransferComponent->QueryRangeAttribute(AttributeIDTag, RangeAttribute))
		{
			const float LowerValue = RangeAttribute.Value - RangeAttribute.Value * RangeAttribute.DownwardFloatingRatio;
			const float UpperValue = RangeAttribute.Value + RangeAttribute.Value * RangeAttribute.UpwardFloatingRatio;
			return FMath::FRandRange(FMath::Min(LowerValue, UpperValue), FMath::Max(LowerValue, UpperValue));
		}
		return GetAttributeSingleValue(DataTransferComponent, AttributeIDTag);
	}

	void NormalizeDamageEffect(FLxDamageEffect& InOutDamageEffect)
	{
		if (InOutDamageEffect.DamageValues.IsEmpty() && InOutDamageEffect.DamageValue > 0.f)
		{
			FLxDamageValue DamageValue;
			DamageValue.DamageTypeTag = LxTag_CommonEffect_DamageEffect_Normal;
			DamageValue.DamageValue = InOutDamageEffect.DamageValue;
			InOutDamageEffect.DamageValues.Add(DamageValue);
		}
	}

	bool ResolveDamageValueFromSourceAttribute(ULxCharacterDataTransferComponent* SourceDataTransferComponent, FLxDamageValue& InOutDamageValue)
	{
		if (!InOutDamageValue.SourceAttributeIDTag.IsValid())
		{
			return false;
		}

		float SourceAttributeValue = 0.f;
		if (SourceDataTransferComponent == nullptr || !SourceDataTransferComponent->QueryCharacterAttributeValue(InOutDamageValue.SourceAttributeIDTag, SourceAttributeValue))
		{
			InOutDamageValue.DamageValue = 0.f;
			return true;
		}

		const float SourceValue = FMath::Max(0.f, GetDamageSourceValue(SourceDataTransferComponent, InOutDamageValue.SourceAttributeIDTag));
		InOutDamageValue.DamageValue = SourceValue * FMath::Max(0.f, InOutDamageValue.SourceAttributeRatio);
		return true;
	}

	float RefreshDamageEffectTotalValue(FLxDamageEffect& InOutDamageEffect)
	{
		NormalizeDamageEffect(InOutDamageEffect);
		float TotalDamageValue = 0.f;
		for (const FLxDamageValue& DamageValue : InOutDamageEffect.DamageValues)
		{
			TotalDamageValue += FMath::Max(0.f, DamageValue.DamageValue);
		}

		InOutDamageEffect.DamageValue = TotalDamageValue;
		return TotalDamageValue;
	}

	float GetTotalDamageValue(const TArray<FLxDamageEffect>& DamageEffects)
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

	void EnsureOutputPackageMetadata(FLxDamageCalculationContext& InOutContext)
	{
		if (InOutContext.OutputEffectPackage.SourceContext.SourceType == ELxEffectPackageSource::None)
		{
			InOutContext.OutputEffectPackage.SourceContext = InOutContext.InputEffectPackage.SourceContext;
		}

		if (InOutContext.OutputEffectPackage.SourceContext.SourceActor == nullptr)
		{
			InOutContext.OutputEffectPackage.SourceContext.SourceActor = InOutContext.SourceActor;
		}

		if (InOutContext.OutputEffectPackage.TargetActor == nullptr)
		{
			InOutContext.OutputEffectPackage.TargetActor = InOutContext.TargetActor;
		}

		InOutContext.OutputEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
	}
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateOutgoingDamage_Implementation(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = GenerateDamageFromSourceAttributes(InDamageContext);
	if (ResultContext.DamageEffects.IsEmpty())
	{
		ResultContext = CalculateAttackPowerDamageOutput(ResultContext);
	}

	ResultContext = CalculateCriticalDamageOutput(ResultContext);
	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateIncomingDamage_Implementation(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = GenerateDamageFromSourceAttributes(InDamageContext);
	ResultContext = CalculateDefenseDamageReduction(ResultContext);
	ResultContext = ApplyShieldHealthDamageSettlement(ResultContext);
	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::GenerateDamageFromSourceAttributes(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;
	if (ResultContext.DamageEffects.IsEmpty())
	{
		ResultContext.DamageEffects = ResultContext.InputEffectPackage.DamageEffects;
	}

	bool bGeneratedAnyDamage = false;
	for (FLxDamageEffect& DamageEffect : ResultContext.DamageEffects)
	{
		ResultContext.bCriticalHit |= DamageEffect.bCriticalHit;

		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			bGeneratedAnyDamage |= ResolveDamageValueFromSourceAttribute(ResultContext.SourceDataTransferComponent, DamageValue);
		}

		RefreshDamageEffectTotalValue(DamageEffect);
	}

	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateAttackPowerDamageOutput(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;

	const float RolledDamageValue = FMath::Max(0.f, GetDamageSourceValue(ResultContext.SourceDataTransferComponent, LxTag_Attribute_Range_AttackPower));
	if (FMath::IsNearlyZero(RolledDamageValue))
	{
		return ResultContext;
	}

	FLxDamageValue DamageValue;
	DamageValue.DamageTypeTag = LxTag_CommonEffect_DamageEffect_Normal;
	DamageValue.DamageValue = RolledDamageValue;

	FLxDamageEffect DamageEffect;
	DamageEffect.TargetAttributeIDTag = LxTag_Attribute_Resource_Health;
	DamageEffect.DamageValue = RolledDamageValue;
	DamageEffect.DamageValues.Add(DamageValue);
	ResultContext.DamageEffects.Add(DamageEffect);
	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateCriticalDamageOutput(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;
	if (ResultContext.DamageEffects.IsEmpty())
	{
		return ResultContext;
	}

	const float CriticalChance = FMath::Clamp(GetAttributeSingleValue(ResultContext.SourceDataTransferComponent, LxTag_Attribute_Judgement_CriticalChance), 0.f, 1.f);
	if (FMath::FRand() > CriticalChance)
	{
		return ResultContext;
	}

	float CriticalDamageMultiplier = DefaultCriticalDamageMultiplier;
	const float CriticalDamagePercent = GetAttributeSingleValue(ResultContext.SourceDataTransferComponent, LxTag_Attribute_Percentage_CriticalDamage, 0.f);
	if (CriticalDamagePercent > 0.f)
	{
		CriticalDamageMultiplier = 1.f + CriticalDamagePercent;
	}

	CriticalDamageMultiplier = FMath::Max(1.f, CriticalDamageMultiplier);
	ResultContext.bCriticalHit = true;
	for (FLxDamageEffect& DamageEffect : ResultContext.DamageEffects)
	{
		NormalizeDamageEffect(DamageEffect);
		DamageEffect.bCriticalHit = true;
		DamageEffect.DamageValue *= CriticalDamageMultiplier;
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			DamageValue.DamageValue *= CriticalDamageMultiplier;
		}
	}

	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateDefenseDamageReduction(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;
	const float DefenseValue = FMath::Max(0.f, GetAttributeSingleValue(ResultContext.TargetDataTransferComponent, LxTag_Attribute_Numeric_Armor));
	ResultContext.DefenseValue = DefenseValue;
	if (FMath::IsNearlyZero(DefenseValue))
	{
		return ResultContext;
	}

	for (FLxDamageEffect& DamageEffect : ResultContext.DamageEffects)
	{
		NormalizeDamageEffect(DamageEffect);
		float TotalReducedDamageValue = 0.f;
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			const float SourceDamageValue = FMath::Max(0.f, DamageValue.DamageValue);
			DamageValue.DamageValue = SourceDamageValue > 0.f ? SourceDamageValue * SourceDamageValue / (SourceDamageValue + DefenseValue) : 0.f;
			TotalReducedDamageValue += DamageValue.DamageValue;
		}
		DamageEffect.DamageValue = TotalReducedDamageValue;
	}

	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::ApplyShieldHealthDamageSettlement(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;

	const float TotalDamageValue = GetTotalDamageValue(ResultContext.DamageEffects);
	if (FMath::IsNearlyZero(TotalDamageValue))
	{
		return ResultContext;
	}

	const float ShieldValue = FMath::Max(0.f, GetAttributeSingleValue(ResultContext.TargetDataTransferComponent, LxTag_Attribute_Resource_Shield));
	ResultContext.ShieldDamageValue = FMath::Min(ShieldValue, TotalDamageValue);
	ResultContext.HealthDamageValue = FMath::Max(0.f, TotalDamageValue - ResultContext.ShieldDamageValue);
	return ResultContext;
}
