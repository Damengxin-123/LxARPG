#include "LxDefaultDamageCalculationNodes.h"

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
}

ULxDamageAttackPowerOutputNode::ULxDamageAttackPowerOutputNode()
{
	DamageTypeTag = LxTag_CommonEffect_DamageEffect_Normal;
}

void ULxDamageAttackPowerOutputNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	FLxAttributeValue AttackPowerValue;
	if (!QueryCalculatedAttributeValue(InOutContext.SourceDataTransferComponent, LxTag_Attribute_Numeric_AttackPower, AttackPowerValue))
	{
		return;
	}

	const float RolledDamageValue = FMath::Max(0.f, RollRangedAttributeValue(AttackPowerValue));
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

	const float CriticalChance = FMath::Clamp(GetAttributeSingleValue(InOutContext.SourceDataTransferComponent, LxTag_Attribute_Judgement_CriticalChance), 0.f, 100.f);
	if (FMath::FRandRange(0.f, 100.f) > CriticalChance)
	{
		return;
	}

	float CriticalDamageMultiplier = DefaultCriticalDamageMultiplier;
	const float CriticalDamagePercent = GetAttributeSingleValue(InOutContext.SourceDataTransferComponent, LxTag_Attribute_Percentage_CriticalDamage, 0.f);
	if (CriticalDamagePercent > 0.f)
	{
		CriticalDamageMultiplier = 1.f + CriticalDamagePercent * 0.01f;
	}

	CriticalDamageMultiplier = FMath::Max(1.f, CriticalDamageMultiplier);
	InOutContext.bCriticalHit = true;
	for (FLxDamageEffect& DamageEffect : InOutContext.DamageEffects)
	{
		NormalizeDamageEffect(DamageEffect);
		DamageEffect.DamageValue *= CriticalDamageMultiplier;
		for (FLxDamageValue& DamageValue : DamageEffect.DamageValues)
		{
			DamageValue.DamageValue *= CriticalDamageMultiplier;
		}
	}
}

void ULxDamageDefenseReceiveNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	const float DefenseValue = FMath::Max(0.f, GetAttributeSingleValue(InOutContext.TargetDataTransferComponent, LxTag_Attribute_Numeric_Armor));
	InOutContext.DefenseValue = DefenseValue;
	if (FMath::IsNearlyZero(DefenseValue))
	{
		return;
	}

	for (FLxDamageEffect& DamageEffect : InOutContext.DamageEffects)
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
}

void ULxDamageShieldHealthSettlementNode::ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext)
{
	const float TotalDamageValue = GetTotalDamageValue(InOutContext.DamageEffects);
	if (FMath::IsNearlyZero(TotalDamageValue))
	{
		return;
	}

	const float ShieldValue = FMath::Max(0.f, GetAttributeSingleValue(InOutContext.TargetDataTransferComponent, LxTag_Attribute_Resource_Shield));
	InOutContext.ShieldDamageValue = FMath::Min(ShieldValue, TotalDamageValue);
	InOutContext.HealthDamageValue = FMath::Max(0.f, TotalDamageValue - InOutContext.ShieldDamageValue);

	if (InOutContext.ShieldDamageValue > 0.f)
	{
		FLxAttributeRecoveryEffect ShieldDamageEffect;
		ShieldDamageEffect.AttributeIDTag = LxTag_Attribute_Resource_Shield;
		ShieldDamageEffect.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
		ShieldDamageEffect.RecoveryValue = -InOutContext.ShieldDamageValue;
		InOutContext.OutputEffectPackage.AttributeRecoveryEffects.Add(ShieldDamageEffect);
	}

	if (InOutContext.HealthDamageValue > 0.f)
	{
		FLxAttributeRecoveryEffect HealthDamageEffect;
		HealthDamageEffect.AttributeIDTag = LxTag_Attribute_Resource_Health;
		HealthDamageEffect.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
		HealthDamageEffect.RecoveryValue = -InOutContext.HealthDamageValue;
		InOutContext.OutputEffectPackage.AttributeRecoveryEffects.Add(HealthDamageEffect);
	}
}
