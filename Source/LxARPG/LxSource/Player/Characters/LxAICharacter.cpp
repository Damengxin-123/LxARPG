#include "LxAICharacter.h"

#include "Components/WidgetComponent.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorModule.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIControlComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Player/Controllers/LxAIController.h"
#include "LxARPG/LxSource/UI/WorldSpace/AICharacterInfo/LxAICharacterInfoWidget.h"

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
	AIControlComponent = CreateDefaultSubobject<ULxAIControlComponent>(TEXT("AI操控组件"));

	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::NoThreat,
		{ELxAIActionType::Patrol, ELxAIActionType::Alert}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::Advantage,
		{ELxAIActionType::Heal, ELxAIActionType::Attack, ELxAIActionType::Defend}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::Balanced,
		{ELxAIActionType::Heal, ELxAIActionType::Attack, ELxAIActionType::Defend}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::Disadvantage,
		{ELxAIActionType::Heal, ELxAIActionType::Retreat, ELxAIActionType::Defend}));
	AIControlConfig.SituationBehaviorSets.Add(MakeDefaultBehaviorSet(ELxAISituationLevel::SelfDanger,
		{ELxAIActionType::Retreat, ELxAIActionType::Defend}));
}

ULxAIBehaviorModule* ALxAICharacter::GetAIBehaviorComponent() const
{
	return AIControlComponent ? AIControlComponent->GetBehaviorModule() : nullptr;
}

void ALxAICharacter::InitialCharacterInformation()
{
	Super::InitialCharacterInformation();
	if (ULxCharacterEffectComponent* EffectComponent = GetCharacterEffectComponent())
	{
		EffectComponent->OnCharacterDamageReceived.RemoveDynamic(this, &ALxAICharacter::HandleAIReceivedDamage);
		EffectComponent->OnCharacterDamageReceived.AddDynamic(this, &ALxAICharacter::HandleAIReceivedDamage);
	}
	BindCharacterInfoWidgets();
	if (AIControlComponent)
	{
		AIControlComponent->BaseComponentInitialize();
	}
}

void ALxAICharacter::HandleAIReceivedDamage(const FLxDamageReceiveResult&, AActor* AttackerActor)
{
	if (ALxAIController* AIController = Cast<ALxAIController>(GetController());
		AIController && IsValid(AttackerActor) && AttackerActor != this)
	{
		// 不依赖伤害感知的异步派发，保证低生命决策的这一轮已经有可用逃跑目标。
		AIController->ReportPerceivedTarget(AttackerActor, ELxAIPerceptionSource::Damage, true);
	}
}

void ALxAICharacter::HandleAIAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	RefreshCharacterInfoWidgetsHealth();
}

void ALxAICharacter::BindCharacterInfoWidgets()
{
	if (ULxCharacterAttributeComponent* AttributeComponent = GetCharacterAttributeComponent())
	{
		AttributeComponent->OnTypedAttributeSnapshotChanged.RemoveDynamic(this, &ALxAICharacter::HandleAIAttributesChanged);
		AttributeComponent->OnTypedAttributeSnapshotChanged.AddDynamic(this, &ALxAICharacter::HandleAIAttributesChanged);
	}

	RefreshCharacterInfoWidgetsHealth();
}

void ALxAICharacter::RefreshCharacterInfoWidgetsHealth() const
{
	TInlineComponentArray<UWidgetComponent*> WidgetComponents(this);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (!IsValid(WidgetComponent))
		{
			continue;
		}

		WidgetComponent->InitWidget();
		if (ULxAICharacterInfoWidget* CharacterInfoWidget = Cast<ULxAICharacterInfoWidget>(WidgetComponent->GetUserWidgetObject()))
		{
			CharacterInfoWidget->UpdateAIHealthPercent(GetCurrentHealthRatio());
		}
	}
}

ELxAITargetRelation ALxAICharacter::ResolveBaseTargetRelation(const ALxBaseCharacter* InTargetCharacter) const
{
	if (!IsValid(InTargetCharacter) || InTargetCharacter == this)
	{
		return ELxAITargetRelation::Ignore;
	}

	const ULxCharacterAttributeComponent* SpecialAttributeComponent = GetCharacterAttributeComponent();
	if (!SpecialAttributeComponent)
	{
		return ELxAITargetRelation::Ignore;
	}

	switch (SpecialAttributeComponent->GetCharacterFactionRelation(InTargetCharacter))
	{
	case ELxCharacterFactionRelation::Friendly:
		return ELxAITargetRelation::Assist;
	case ELxCharacterFactionRelation::Hostile:
		return ELxAITargetRelation::Hostile;
	default:
		return ELxAITargetRelation::Ignore;
	}
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
