#include "LxAIBehaviorComponent.h"

#include "AIController.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

namespace
{
	/** 逃跑行为每次重新寻路时使用的邻近移动步长，避免远距离终点越出导航区域。 */
	constexpr float RetreatMoveStepDistance = 300.0f;

	/** 逃跑目标点投射失败时逐级缩短距离的尝试比例。 */
	constexpr float RetreatDistanceScales[] = {1.0f, 0.75f, 0.5f, 0.25f};

	/** 主逃跑方向不可导航时在后方扇区内按左右交替顺序尝试的水平偏转角度。 */
	constexpr float RetreatDirectionAngles[] = {0.0f, 25.0f, -25.0f, 50.0f, -50.0f, 70.0f, -70.0f};

	/** 导航路径点距离比较允许的微小误差，避免导航投射误差导致有效路径被拒绝。 */
	constexpr float RetreatPathDistanceTolerance = 5.0f;

	/** 判断导航路径中的每一段是否都没有向追逐中的最近敌方靠近。 */
	bool IsRetreatPathMovingAwayFromEnemy(const UNavigationPath* InNavigationPath, const FVector& InEnemyLocation)
	{
		if (!IsValid(InNavigationPath) || !InNavigationPath->IsValid() || InNavigationPath->IsPartial() ||
			InNavigationPath->PathPoints.Num() < 2)
		{
			return false;
		}

		float PreviousDistance = FVector::Dist2D(InNavigationPath->PathPoints[0], InEnemyLocation);
		for (int32 PointIndex = 1; PointIndex < InNavigationPath->PathPoints.Num(); ++PointIndex)
		{
			const float CurrentDistance = FVector::Dist2D(
				InNavigationPath->PathPoints[PointIndex], InEnemyLocation);
			if (CurrentDistance + RetreatPathDistanceTolerance < PreviousDistance)
			{
				return false;
			}
			PreviousDistance = CurrentDistance;
		}
		return true;
	}
}

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

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteBehavior(const ELxAIActionType InActionType,
	const FLxAIBattleSnapshot& InBattleSnapshot)
{
	if (!OwnerAICharacter || !CharacterMoveComponent)
	{
		BaseComponentInitialize();
		if (!OwnerAICharacter || !CharacterMoveComponent)
		{
			return ELxAIBehaviorExecutionResult::Failed;
		}
	}
	if (!CanExecuteBehavior(InActionType, InBattleSnapshot))
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}

	switch (InActionType)
	{
	case ELxAIActionType::Patrol:
		return ExecutePatrol();
	case ELxAIActionType::Alert:
		return ExecuteAlert(InBattleSnapshot);
	case ELxAIActionType::Attack:
		return ExecuteAttack(InBattleSnapshot);
	case ELxAIActionType::Defend:
		return ExecuteDefend(InBattleSnapshot);
	case ELxAIActionType::Heal:
		return ExecuteHeal(InBattleSnapshot);
	case ELxAIActionType::Retreat:
		return ExecuteRetreat(InBattleSnapshot);
	default:
		StopBehavior();
		return ELxAIBehaviorExecutionResult::Failed;
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
	if (!OwnerAICharacter || !CharacterMoveComponent || !IsValid(InBattleSnapshot.HighestThreatEnemy))
	{
		return false;
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const ULxSkillBackpackComponent* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	return Config.AttackSkillItemId.IsValid() && SkillBackpack && OwnerAICharacter->GetSkillCastComponent() &&
		SkillBackpack->FindSkillItemByTagID(Config.AttackSkillItemId);
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
	return OwnerAICharacter && CharacterMoveComponent && InBattleSnapshot.bHasThreat &&
		IsValid(InBattleSnapshot.NearestEnemy);
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

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecutePatrol()
{
	if (!CharacterMoveComponent || CharacterMoveComponent->IsNavigationMoving())
	{
		return CharacterMoveComponent ? ELxAIBehaviorExecutionResult::InProgress :
			ELxAIBehaviorExecutionResult::Failed;
	}

	FNavLocation PatrolLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		if (NavigationSystem->GetRandomReachablePointInRadius(PatrolOrigin,
			OwnerAICharacter->GetAIControlConfig().PatrolRadius, PatrolLocation))
		{
			return CharacterMoveComponent->RequestMoveToLocation(PatrolLocation.Location, 50.0f) ?
				ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
		}
	}
	return ELxAIBehaviorExecutionResult::Failed;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot)
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
	return ELxAIBehaviorExecutionResult::InProgress;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	AActor* TargetActor = InBattleSnapshot.HighestThreatEnemy;
	if (!IsValid(TargetActor))
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->SetFocus(TargetActor);
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const float Distance = FVector::Dist(OwnerAICharacter->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > Config.AttackSkillRange)
	{
		const float AcceptanceRadius = FMath::Min(Config.AttackAcceptanceRadius, Config.AttackSkillRange);
		return CharacterMoveComponent->RequestMoveToActor(TargetActor, AcceptanceRadius) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}

	CharacterMoveComponent->StopActiveMovement();
	ULxSkillBackpackComponent* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	ULxSkillCastComponent* SkillCast = OwnerAICharacter->GetSkillCastComponent();
	if (!SkillBackpack || !SkillCast)
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}
	if (!SkillCast->IsSkillCastIdle())
	{
		return ELxAIBehaviorExecutionResult::Waiting;
	}
	ULxSkillItem* SkillItem = SkillBackpack->FindSkillItemByTagID(Config.AttackSkillItemId);
	if (!SkillItem)
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}
	const FVector AimDirection = (TargetActor->GetActorLocation() - OwnerAICharacter->GetActorLocation()).GetSafeNormal();
	const FLxSkillCastContext CastContext = SkillCast->MakeSkillCastContext(OwnerAICharacter, TargetActor,
		TargetActor->GetActorLocation(), true, AimDirection, true);
	return SkillCast->ReleaseSkillItemDirectly(SkillItem, CastContext) ?
		ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot)
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
		return CharacterMoveComponent->RequestMoveToLocation(InBattleSnapshot.AssistCenter, 150.0f) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}
	CharacterMoveComponent->StopActiveMovement();
	return ELxAIBehaviorExecutionResult::InProgress;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	AActor* HealTarget = InBattleSnapshot.LowestStateAlly;
	if (!IsValid(HealTarget))
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->SetFocus(HealTarget);
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const float Distance = FVector::Dist(OwnerAICharacter->GetActorLocation(), HealTarget->GetActorLocation());
	if (Distance > Config.HealSkillRange)
	{
		return CharacterMoveComponent->RequestMoveToActor(HealTarget, Config.HealSkillRange * 0.8f) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}

	CharacterMoveComponent->StopActiveMovement();
	ULxSkillBackpackComponent* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	ULxSkillCastComponent* SkillCast = OwnerAICharacter->GetSkillCastComponent();
	if (!SkillBackpack || !SkillCast)
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}
	if (!SkillCast->IsSkillCastIdle())
	{
		return ELxAIBehaviorExecutionResult::Waiting;
	}
	ULxSkillItem* SkillItem = SkillBackpack->FindSkillItemByTagID(Config.HealSkillItemId);
	if (!SkillItem)
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}
	const FVector AimDirection = (HealTarget->GetActorLocation() - OwnerAICharacter->GetActorLocation()).GetSafeNormal();
	const FLxSkillCastContext CastContext = SkillCast->MakeSkillCastContext(OwnerAICharacter, HealTarget,
		HealTarget->GetActorLocation(), true, AimDirection, true);
	return SkillCast->ReleaseSkillItemDirectly(SkillItem, CastContext) ?
		ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	AActor* NearestEnemy = InBattleSnapshot.NearestEnemy;
	if (!IsValid(NearestEnemy))
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}

	const FVector CurrentLocation = OwnerAICharacter->GetActorLocation();
	const FVector EnemyLocation = NearestEnemy->GetActorLocation();
	FVector RetreatDirection = (CurrentLocation - EnemyLocation).GetSafeNormal2D();
	if (RetreatDirection.IsNearlyZero())
	{
		RetreatDirection = -OwnerAICharacter->GetActorForwardVector();
	}

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		for (const float DirectionAngle : RetreatDirectionAngles)
		{
			const FVector CandidateDirection = RetreatDirection.RotateAngleAxis(DirectionAngle, FVector::UpVector);
			for (const float DistanceScale : RetreatDistanceScales)
			{
				const FVector DesiredLocation = CurrentLocation +
					CandidateDirection * RetreatMoveStepDistance * DistanceScale;
				FNavLocation ReachableLocation;
				if (!NavigationSystem->ProjectPointToNavigation(DesiredLocation, ReachableLocation))
				{
					continue;
				}
				if (FVector::DistSquared2D(ReachableLocation.Location, EnemyLocation) <=
					FVector::DistSquared2D(CurrentLocation, EnemyLocation))
				{
					continue;
				}

				const UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(
					this, CurrentLocation, ReachableLocation.Location, OwnerAICharacter);
				if (!IsRetreatPathMovingAwayFromEnemy(NavigationPath, EnemyLocation))
				{
					continue;
				}

				if (AAIController* AIController = GetOwnerAIController())
				{
					AIController->ClearFocus(EAIFocusPriority::Gameplay);
				}
				if (CharacterMoveComponent->RequestMoveToLocation(ReachableLocation.Location, 35.0f))
				{
					return ELxAIBehaviorExecutionResult::Started;
				}
			}
		}
	}
	return ELxAIBehaviorExecutionResult::Failed;
}

AAIController* ULxAIBehaviorComponent::GetOwnerAIController() const
{
	return OwnerAICharacter ? Cast<AAIController>(OwnerAICharacter->GetController()) : nullptr;
}
