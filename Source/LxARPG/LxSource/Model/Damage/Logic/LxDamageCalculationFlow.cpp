#include "LxDamageCalculationFlow.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageTags.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

namespace
{
	bool QueryCalculatedAttributeValue(ULxCharacterDataTransferComponent* DataTransferComponent, FGameplayTag AttributeIDTag, FLxAttributeValue& OutAttributeValue)
	{
		if (DataTransferComponent == nullptr || !AttributeIDTag.IsValid())
		{
			return false;
		}

		FLxAttributeData AttributeData;
		if (!DataTransferComponent->QueryCharacterAttributeByIDTag(AttributeIDTag, AttributeData))
		{
			return false;
		}

		OutAttributeValue = AttributeData.CalculatedAttributeValue;
		return true;
	}

	float GetAttributeSingleValue(ULxCharacterDataTransferComponent* DataTransferComponent, FGameplayTag AttributeIDTag, float DefaultValue = 0.f)
	{
		FLxAttributeValue AttributeValue;
		return QueryCalculatedAttributeValue(DataTransferComponent, AttributeIDTag, AttributeValue) ? AttributeValue.Value : DefaultValue;
	}

	float RollRangedAttributeValue(const FLxAttributeValue& AttributeValue)
	{
		const float LowerValue = FMath::Min(AttributeValue.Value, AttributeValue.ValueLimit);
		const float UpperValue = FMath::Max(AttributeValue.Value, AttributeValue.ValueLimit);
		if (UpperValue > LowerValue)
		{
			return FMath::FRandRange(LowerValue, UpperValue);
		}

		return AttributeValue.Value;
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

		FLxAttributeValue SourceAttributeValue;
		if (!QueryCalculatedAttributeValue(SourceDataTransferComponent, InOutDamageValue.SourceAttributeIDTag, SourceAttributeValue))
		{
			InOutDamageValue.DamageValue = 0.f;
			return true;
		}

		const float SourceValue = FMath::Max(0.f, RollRangedAttributeValue(SourceAttributeValue));
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
	EnsureOutputPackageMetadata(ResultContext);
	if (ResultContext.DamageEffects.IsEmpty())
	{
		ResultContext.DamageEffects = ResultContext.InputEffectPackage.DamageEffects;
	}

	bool bGeneratedAnyDamage = false;
	for (FLxDamageEffect& DamageEffect : ResultContext.DamageEffects)
	{
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			bGeneratedAnyDamage |= ResolveDamageValueFromSourceAttribute(ResultContext.SourceDataTransferComponent, DamageValue);
		}

		RefreshDamageEffectTotalValue(DamageEffect);
	}

	ResultContext.OutputEffectPackage.DamageEffects = ResultContext.DamageEffects;
	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateAttackPowerDamageOutput(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;
	EnsureOutputPackageMetadata(ResultContext);

	FLxAttributeValue AttackPowerValue;
	if (!QueryCalculatedAttributeValue(ResultContext.SourceDataTransferComponent, LxTag_Attribute_Numeric_AttackPower, AttackPowerValue))
	{
		return ResultContext;
	}

	const float RolledDamageValue = FMath::Max(0.f, RollRangedAttributeValue(AttackPowerValue));
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
	ResultContext.OutputEffectPackage.DamageEffects = ResultContext.DamageEffects;
	return ResultContext;
}

FLxDamageCalculationContext ULxDamageCalculationFlow::CalculateCriticalDamageOutput(const FLxDamageCalculationContext& InDamageContext)
{
	FLxDamageCalculationContext ResultContext = InDamageContext;
	if (ResultContext.DamageEffects.IsEmpty())
	{
		return ResultContext;
	}

	const float CriticalChance = FMath::Clamp(GetAttributeSingleValue(ResultContext.SourceDataTransferComponent, LxTag_Attribute_Judgement_CriticalChance), 0.f, 100.f);
	if (FMath::FRandRange(0.f, 100.f) > CriticalChance)
	{
		ResultContext.OutputEffectPackage.DamageEffects = ResultContext.DamageEffects;
		return ResultContext;
	}

	float CriticalDamageMultiplier = DefaultCriticalDamageMultiplier;
	const float CriticalDamagePercent = GetAttributeSingleValue(ResultContext.SourceDataTransferComponent, LxTag_Attribute_Percentage_CriticalDamage, 0.f);
	if (CriticalDamagePercent > 0.f)
	{
		CriticalDamageMultiplier = 1.f + CriticalDamagePercent * 0.01f;
	}

	CriticalDamageMultiplier = FMath::Max(1.f, CriticalDamageMultiplier);
	ResultContext.bCriticalHit = true;
	for (FLxDamageEffect& DamageEffect : ResultContext.DamageEffects)
	{
		NormalizeDamageEffect(DamageEffect);
		DamageEffect.DamageValue *= CriticalDamageMultiplier;
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			DamageValue.DamageValue *= CriticalDamageMultiplier;
		}
	}

	ResultContext.OutputEffectPackage.DamageEffects = ResultContext.DamageEffects;
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
	EnsureOutputPackageMetadata(ResultContext);
	ResultContext.OutputEffectPackage.DamageEffects.Reset();
	ResultContext.OutputEffectPackage.AttributeRecoveryEffects.Reset();

	const float TotalDamageValue = GetTotalDamageValue(ResultContext.DamageEffects);
	if (FMath::IsNearlyZero(TotalDamageValue))
	{
		return ResultContext;
	}

	const float ShieldValue = FMath::Max(0.f, GetAttributeSingleValue(ResultContext.TargetDataTransferComponent, LxTag_Attribute_Resource_Shield));
	ResultContext.ShieldDamageValue = FMath::Min(ShieldValue, TotalDamageValue);
	ResultContext.HealthDamageValue = FMath::Max(0.f, TotalDamageValue - ResultContext.ShieldDamageValue);

	if (ResultContext.ShieldDamageValue > 0.f)
	{
		FLxAttributeRecoveryEffect ShieldDamageEffect;
		ShieldDamageEffect.AttributeIDTag = LxTag_Attribute_Resource_Shield;
		ShieldDamageEffect.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
		ShieldDamageEffect.RecoveryValue = -ResultContext.ShieldDamageValue;
		ResultContext.OutputEffectPackage.AttributeRecoveryEffects.Add(ShieldDamageEffect);
	}

	if (ResultContext.HealthDamageValue > 0.f)
	{
		FLxAttributeRecoveryEffect HealthDamageEffect;
		HealthDamageEffect.AttributeIDTag = LxTag_Attribute_Resource_Health;
		HealthDamageEffect.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
		HealthDamageEffect.RecoveryValue = -ResultContext.HealthDamageValue;
		ResultContext.OutputEffectPackage.AttributeRecoveryEffects.Add(HealthDamageEffect);
	}

	if (ResultContext.bApplyResultToTarget && ResultContext.TargetDataTransferComponent != nullptr && !ResultContext.OutputEffectPackage.AttributeRecoveryEffects.IsEmpty())
	{
		ResultContext.TargetDataTransferComponent->ReceiveEffectPackage(ResultContext.OutputEffectPackage);
	}

	return ResultContext;
}
