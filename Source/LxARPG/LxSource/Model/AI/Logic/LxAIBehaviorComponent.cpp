#include "LxAIBehaviorComponent.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

ULxAIBehaviorComponent::ULxAIBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxAIBehaviorComponent::BaseComponentInitialize()
{
	OwnerAICharacter = Cast<ALxAICharacter>(GetOwner());
	CharacterMoveComponent = OwnerAICharacter ? OwnerAICharacter->GetCharacterMoveComponent() : nullptr;
	if (OwnerAICharacter)
	{
		PatrolOrigin = OwnerAICharacter->GetActorLocation();
	}
}

bool ULxAIBehaviorComponent::CanExecuteBehavior(const ELxAIActionType InActionType,
	const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	switch (InActionType)
	{
	case ELxAIActionType::Patrol:
		return CanExecutePatrol(InBattleSnapshot);
	case ELxAIActionType::Alert:
		return CanExecuteAlert(InBattleSnapshot);
	case ELxAIActionType::Attack:
		return CanExecuteAttack(InBattleSnapshot);
	case ELxAIActionType::Defend:
		return CanExecuteDefend(InBattleSnapshot);
	case ELxAIActionType::Heal:
		return CanExecuteHeal(InBattleSnapshot);
	case ELxAIActionType::Retreat:
		return CanExecuteRetreat(InBattleSnapshot);
	default:
		return false;
	}
}

void ULxAIBehaviorComponent::ExecuteBehavior(const ELxAIActionType InActionType,
	const FLxAIBattleSnapshot& InBattleSnapshot)
{
	if (!OwnerAICharacter || !CharacterMoveComponent)
	{
		BaseComponentInitialize();
		if (!OwnerAICharacter || !CharacterMoveComponent)
		{
			return;
		}
	}
	if (!CanExecuteBehavior(InActionType, InBattleSnapshot))
	{
		return;
	}

	switch (InActionType)
	{
	case ELxAIActionType::Patrol:
		ExecutePatrol();
		break;
	case ELxAIActionType::Alert:
		ExecuteAlert(InBattleSnapshot);
		break;
	case ELxAIActionType::Attack:
		ExecuteAttack(InBattleSnapshot);
		break;
	case ELxAIActionType::Defend:
		ExecuteDefend(InBattleSnapshot);
		break;
	case ELxAIActionType::Heal:
		ExecuteHeal(InBattleSnapshot);
		break;
	case ELxAIActionType::Retreat:
		ExecuteRetreat(InBattleSnapshot);
		break;
	default:
		StopBehavior();
		break;
	}
}

bool ULxAIBehaviorComponent::CanExecutePatrol(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterMoveComponent && !InBattleSnapshot.bHasThreat;
}

bool ULxAIBehaviorComponent::CanExecuteAlert(const FLxAIBattleSnapshot&) const
{
	return OwnerAICharacter && CharacterMoveComponent;
}

bool ULxAIBehaviorComponent::CanExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterMoveComponent && IsValid(InBattleSnapshot.HighestThreatEnemy);
}

bool ULxAIBehaviorComponent::CanExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterMoveComponent && InBattleSnapshot.bHasThreat;
}

bool ULxAIBehaviorComponent::CanExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	if (!OwnerAICharacter || !CharacterMoveComponent || !IsValid(InBattleSnapshot.LowestStateAlly))
	{
		return false;
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const ULxSkillBackpackComponent* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	return Config.HealSkillItemId.IsValid() && SkillBackpack && OwnerAICharacter->GetSkillCastComponent() &&
		SkillBackpack->FindSkillItemByTagID(Config.HealSkillItemId);
}

bool ULxAIBehaviorComponent::CanExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterMoveComponent && InBattleSnapshot.bHasThreat;
}

void ULxAIBehaviorComponent::StopBehavior()
{
	if (CharacterMoveComponent)
	{
		CharacterMoveComponent->StopActiveMovement();
	}
	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void ULxAIBehaviorComponent::ExecutePatrol()
{
	if (!CharacterMoveComponent || CharacterMoveComponent->IsNavigationMoving())
	{
		return;
	}

	FNavLocation PatrolLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		if (NavigationSystem->GetRandomReachablePointInRadius(PatrolOrigin,
			OwnerAICharacter->GetAIControlConfig().PatrolRadius, PatrolLocation))
		{
			CharacterMoveComponent->RequestMoveToLocation(PatrolLocation.Location, 50.0f);
		}
	}
}

void ULxAIBehaviorComponent::ExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	CharacterMoveComponent->StopActiveMovement();
	if (AAIController* AIController = GetOwnerAIController())
	{
		if (IsValid(InBattleSnapshot.HighestThreatEnemy))
		{
			AIController->SetFocus(InBattleSnapshot.HighestThreatEnemy);
		}
		else
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

void ULxAIBehaviorComponent::ExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	AActor* TargetActor = InBattleSnapshot.HighestThreatEnemy;
	if (!IsValid(TargetActor))
	{
		ExecuteAlert(InBattleSnapshot);
		return;
	}

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->SetFocus(TargetActor);
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const float Distance = FVector::Dist(OwnerAICharacter->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > Config.AttackAcceptanceRadius)
	{
		CharacterMoveComponent->RequestMoveToActor(TargetActor, Config.AttackAcceptanceRadius);
	}
	else
	{
		CharacterMoveComponent->StopActiveMovement();
	}

	if (Distance > Config.AttackSkillRange || !Config.AttackSkillItemId.IsValid())
	{
		return;
	}
	ULxSkillBackpackComponent* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	ULxSkillCastComponent* SkillCast = OwnerAICharacter->GetSkillCastComponent();
	if (!SkillBackpack || !SkillCast || !SkillCast->IsSkillCastIdle())
	{
		return;
	}
	if (ULxSkillItem* SkillItem = SkillBackpack->FindSkillItemByTagID(Config.AttackSkillItemId))
	{
		const FVector AimDirection = (TargetActor->GetActorLocation() - OwnerAICharacter->GetActorLocation()).GetSafeNormal();
		const FLxSkillCastContext CastContext = SkillCast->MakeSkillCastContext(OwnerAICharacter, TargetActor,
			TargetActor->GetActorLocation(), true, AimDirection, true);
		SkillCast->ReleaseSkillItemDirectly(SkillItem, CastContext);
	}
}

void ULxAIBehaviorComponent::ExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	if (AAIController* AIController = GetOwnerAIController())
	{
		if (IsValid(InBattleSnapshot.HighestThreatEnemy))
		{
			AIController->SetFocus(InBattleSnapshot.HighestThreatEnemy);
		}
	}
	if (FVector::DistSquared2D(OwnerAICharacter->GetActorLocation(), InBattleSnapshot.AssistCenter) > FMath::Square(250.0f))
	{
		CharacterMoveComponent->RequestMoveToLocation(InBattleSnapshot.AssistCenter, 150.0f);
	}
	else
	{
		CharacterMoveComponent->StopActiveMovement();
	}
}

void ULxAIBehaviorComponent::ExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	AActor* HealTarget = InBattleSnapshot.LowestStateAlly;
	if (!IsValid(HealTarget))
	{
		ExecuteDefend(InBattleSnapshot);
		return;
	}

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->SetFocus(HealTarget);
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const float Distance = FVector::Dist(OwnerAICharacter->GetActorLocation(), HealTarget->GetActorLocation());
	if (Distance > Config.HealSkillRange)
	{
		CharacterMoveComponent->RequestMoveToActor(HealTarget, Config.HealSkillRange * 0.8f);
		return;
	}

	CharacterMoveComponent->StopActiveMovement();
	ULxSkillBackpackComponent* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	ULxSkillCastComponent* SkillCast = OwnerAICharacter->GetSkillCastComponent();
	if (!SkillBackpack || !SkillCast || !SkillCast->IsSkillCastIdle())
	{
		return;
	}
	if (ULxSkillItem* SkillItem = SkillBackpack->FindSkillItemByTagID(Config.HealSkillItemId))
	{
		const FVector AimDirection = (HealTarget->GetActorLocation() - OwnerAICharacter->GetActorLocation()).GetSafeNormal();
		const FLxSkillCastContext CastContext = SkillCast->MakeSkillCastContext(OwnerAICharacter, HealTarget,
			HealTarget->GetActorLocation(), true, AimDirection, true);
		SkillCast->ReleaseSkillItemDirectly(SkillItem, CastContext);
	}
}

void ULxAIBehaviorComponent::ExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	if (CharacterMoveComponent->IsNavigationMoving())
	{
		return;
	}

	FVector RetreatDirection = (OwnerAICharacter->GetActorLocation() - InBattleSnapshot.EnemyCenter).GetSafeNormal2D();
	if (RetreatDirection.IsNearlyZero())
	{
		RetreatDirection = -OwnerAICharacter->GetActorForwardVector();
	}
	const FVector DesiredLocation = OwnerAICharacter->GetActorLocation() +
		RetreatDirection * OwnerAICharacter->GetAIControlConfig().RetreatDistance;
	FNavLocation ReachableLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ReachableLocation))
		{
			if (AAIController* AIController = GetOwnerAIController())
			{
				AIController->ClearFocus(EAIFocusPriority::Gameplay);
			}
			CharacterMoveComponent->RequestMoveToLocation(ReachableLocation.Location, 50.0f);
		}
	}
}

AAIController* ULxAIBehaviorComponent::GetOwnerAIController() const
{
	return OwnerAICharacter ? Cast<AAIController>(OwnerAICharacter->GetController()) : nullptr;
}
