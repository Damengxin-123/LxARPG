#include "LxAICharacter.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Player/Controllers/LxAIController.h"

namespace
{
	/** 创建包含指定战略的默认行为规则。 */
	FLxAIActionRule MakeDefaultRule(const ELxAIActionType InActionType, const ELxAITacticalStrategy InStrategy,
		const float InBaseScore)
	{
		FLxAIActionRule Rule;
		Rule.ActionType = InActionType;
		Rule.AllowedStrategies.Add(InStrategy);
		Rule.BaseScore = InBaseScore;
		return Rule;
	}
}

ALxAICharacter::ALxAICharacter()
{
	AIControllerClass = ALxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	FLxAIActionRule PatrolRule = MakeDefaultRule(ELxAIActionType::Patrol, ELxAITacticalStrategy::Idle, 50.0f);
	PatrolRule.MinExecutionTime = 2.0f;
	AIControlConfig.ActionRules.Add(PatrolRule);

	FLxAIActionRule AlertRule = MakeDefaultRule(ELxAIActionType::Alert, ELxAITacticalStrategy::Idle, 20.0f);
	AIControlConfig.ActionRules.Add(AlertRule);

	FLxAIActionRule AttackRule = MakeDefaultRule(ELxAIActionType::Attack, ELxAITacticalStrategy::Engage, 40.0f);
	AttackRule.AdvantageWeight = 30.0f;
	AttackRule.AssistAdvanceRatioWeight = 15.0f;
	AttackRule.SelfInjuryWeight = -35.0f;
	AttackRule.MinSelfHealthRatio = 0.2f;
	AIControlConfig.ActionRules.Add(AttackRule);

	FLxAIActionRule DefendRule = MakeDefaultRule(ELxAIActionType::Defend, ELxAITacticalStrategy::Engage, 30.0f);
	DefendRule.EnemyAdvanceRatioWeight = 35.0f;
	DefendRule.AssistAdvanceRatioWeight = -15.0f;
	DefendRule.MaxGroupExecutors = 2;
	AIControlConfig.ActionRules.Add(DefendRule);

	FLxAIActionRule HealRule = MakeDefaultRule(ELxAIActionType::Heal, ELxAITacticalStrategy::Engage, 20.0f);
	HealRule.bRequiresInjuredAlly = true;
	HealRule.InjuredAllyWeight = 70.0f;
	HealRule.MaxGroupExecutors = 1;
	HealRule.MinExecutionTime = 1.5f;
	AIControlConfig.ActionRules.Add(HealRule);

	FLxAIActionRule RetreatRule = MakeDefaultRule(ELxAIActionType::Retreat, ELxAITacticalStrategy::Escape, 100.0f);
	RetreatRule.SelfInjuryWeight = 30.0f;
	RetreatRule.MinExecutionTime = 2.0f;
	AIControlConfig.ActionRules.Add(RetreatRule);
}

ELxAITargetRelation ALxAICharacter::ResolveBaseTargetRelation(const ALxBaseCharacter* InTargetCharacter) const
{
	if (!IsValid(InTargetCharacter) || InTargetCharacter == this)
	{
		return ELxAITargetRelation::Ignore;
	}

	const uint8 TargetFactionId = InTargetCharacter->GetFactionId();
	if (TargetFactionId == GetFactionId() || AssistFactionIds.Contains(TargetFactionId))
	{
		return ELxAITargetRelation::Assist;
	}
	if (HostileFactionIds.Contains(TargetFactionId))
	{
		return ELxAITargetRelation::Hostile;
	}
	return ELxAITargetRelation::Ignore;
}

float ALxAICharacter::CalculateEffectiveCombatPower() const
{
	const ULxCharacterAttributeComponent* AttributeComponent = GetCharacterAttributeComponent();
	const float TotalStrength = AttributeComponent ? static_cast<float>(AttributeComponent->CalculateTotalStrength()) : 0.0f;
	return FMath::Max(1.0f, TotalStrength) * GetCurrentHealthRatio() * FMath::Max(0.0f, AIControlConfig.CombatStrengthMultiplier);
}

float ALxAICharacter::GetCurrentHealthRatio() const
{
	const ULxCharacterAttributeComponent* AttributeComponent = GetCharacterAttributeComponent();
	const ULxCharacterBaseAttributeSet* AttributeSet = AttributeComponent ? AttributeComponent->GetRuntimeAttributeSet() : nullptr;
	if (!AttributeSet)
	{
		return 1.0f;
	}

	FLxResourceAttributeData HealthAttribute;
	if (!AttributeSet->GetResourceAttribute(LxTag_Attribute_Resource_Health, HealthAttribute) || HealthAttribute.ValueLimit <= UE_SMALL_NUMBER)
	{
		return 1.0f;
	}
	return FMath::Clamp(HealthAttribute.Value / HealthAttribute.ValueLimit, 0.0f, 1.0f);
}
