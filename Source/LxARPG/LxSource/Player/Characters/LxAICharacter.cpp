#include "LxAICharacter.h"

#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorComponent.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Player/Controllers/LxAIController.h"

namespace
{
	/** 创建指定局势按顺序匹配的默认行为候选集合。 */
	FLxAISituationBehaviorSet MakeDefaultBehaviorSet(const ELxAISituationLevel InSituation,
		std::initializer_list<ELxAIActionType> InBehaviorCandidates)
	{
		FLxAISituationBehaviorSet BehaviorSet;
		BehaviorSet.Situation = InSituation;
		BehaviorSet.BehaviorCandidates.Append(InBehaviorCandidates);
		return BehaviorSet;
	}
}

ALxAICharacter::ALxAICharacter()
{
	AIControllerClass = ALxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIBehaviorComponent = CreateDefaultSubobject<ULxAIBehaviorComponent>(TEXT("AIBehaviorComponent"));

	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::NoThreat,
		{ELxAIActionType::Patrol, ELxAIActionType::Alert}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::Advantage,
		{ELxAIActionType::Heal, ELxAIActionType::Attack, ELxAIActionType::Defend}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::Balanced,
		{ELxAIActionType::Heal, ELxAIActionType::Defend, ELxAIActionType::Attack}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::Disadvantage,
		{ELxAIActionType::Heal, ELxAIActionType::Defend, ELxAIActionType::Retreat}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::SelfDanger,
		{ELxAIActionType::Retreat, ELxAIActionType::Defend}));
}

void ALxAICharacter::InitialCharacterInformation()
{
	Super::InitialCharacterInformation();
	if (AIBehaviorComponent)
	{
		AIBehaviorComponent->BaseComponentInitialize();
	}
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
