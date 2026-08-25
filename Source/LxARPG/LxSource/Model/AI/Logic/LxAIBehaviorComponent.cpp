#include "LxAIBehaviorComponent.h"

#include "AIController.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

namespace
{
	/** 将AI行为配置使用的米换算为虚幻世界单位厘米。 */
	constexpr float AIBehaviorMetersToCentimeters = 100.0f;

	/** 将非负的AI业务距离从米换算为虚幻世界单位厘米。 */
	float ConvertMetersToWorldDistance(const float InDistanceMeters)
	{
		return FMath::Max(0.0f, InDistanceMeters) * AIBehaviorMetersToCentimeters;
	}

	/** 逃跑行为每次重新寻路时使用的邻近移动步长，配置语义为米。 */
	constexpr float RetreatMoveStepDistanceMeters = 3.0f;

	/** 逃跑目标点投射失败时逐级缩短距离的尝试比例。 */
	constexpr float RetreatDistanceScales[] = {1.0f, 0.75f, 0.5f, 0.25f};

	/** 主逃跑方向不可导航时在后方扇区内按左右交替顺序尝试的水平偏转角度。 */
	constexpr float RetreatDirectionAngles[] = {0.0f, 25.0f, -25.0f, 50.0f, -50.0f, 70.0f, -70.0f};

	/** 判断敌方开始追近时允许的距离抖动误差，单位为厘米。 */
	constexpr float RetreatClosingDistanceTolerance = 10.0f;

	/** 判断逃跑候选路径是否完整可达，允许导航系统为了绕过障碍短暂横移或接近敌方。 */
	bool IsCompleteRetreatPath(const UNavigationPath* InNavigationPath)
	{
		return IsValid(InNavigationPath) && InNavigationPath->IsValid() && !InNavigationPath->IsPartial() &&
			InNavigationPath->PathPoints.Num() >= 2;
	}
}

ULxAIBehaviorComponent::ULxAIBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxAIBehaviorComponent::BaseComponentInitialize()
{
	OwnerAICharacter = Cast<ALxAICharacter>(GetOwner());
	CharacterBehaviorControlComponent = OwnerAICharacter ? OwnerAICharacter->GetCharacterBehaviorControlComponent() : nullptr;
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
	if (!OwnerAICharacter || !CharacterBehaviorControlComponent)
	{
		BaseComponentInitialize();
		if (!OwnerAICharacter || !CharacterBehaviorControlComponent)
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
	return OwnerAICharacter && CharacterBehaviorControlComponent && !InBattleSnapshot.bHasThreat;
}

bool ULxAIBehaviorComponent::CanExecuteAlert(const FLxAIBattleSnapshot&) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent;
}

bool ULxAIBehaviorComponent::CanExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	if (!OwnerAICharacter || !CharacterBehaviorControlComponent || !IsValid(InBattleSnapshot.HighestThreatEnemy))
	{
		return false;
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const ULxSkillBackpackModule* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	return Config.AttackSkillItemId.IsValid() && SkillBackpack && OwnerAICharacter->GetSkillCastComponent() &&
		SkillBackpack->FindSkillItemByTagID(Config.AttackSkillItemId);
}

bool ULxAIBehaviorComponent::CanExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent && InBattleSnapshot.bHasThreat;
}

bool ULxAIBehaviorComponent::CanExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	if (!OwnerAICharacter || !CharacterBehaviorControlComponent || !IsValid(InBattleSnapshot.LowestStateAlly))
	{
		return false;
	}
	const FLxAIControlConfig& Config = OwnerAICharacter->GetAIControlConfig();
	const ULxSkillBackpackModule* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	return Config.HealSkillItemId.IsValid() && SkillBackpack && OwnerAICharacter->GetSkillCastComponent() &&
		SkillBackpack->FindSkillItemByTagID(Config.HealSkillItemId);
}

bool ULxAIBehaviorComponent::CanExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent && (bRetreatInProgress ||
		(!bRetreatCompletionBlocked && InBattleSnapshot.bHasThreat && IsValid(InBattleSnapshot.NearestEnemy)));
}

void ULxAIBehaviorComponent::StopBehavior()
{
	if (CharacterBehaviorControlComponent)
	{
		CharacterBehaviorControlComponent->StopActiveMovement();
	}
	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	ResetRetreatState();
}

void ULxAIBehaviorComponent::UpdateRetreatProgress(const FLxAIBattleSnapshot& InBattleSnapshot,
	const bool bInShouldRetreat)
{
	if (!OwnerAICharacter)
	{
		return;
	}

	AActor* NearestEnemy = InBattleSnapshot.NearestEnemy;
	if (bRetreatCompletionBlocked)
	{
		const bool bHasEnemy = InBattleSnapshot.bHasThreat && IsValid(NearestEnemy);
		const float CurrentEnemyDistance = bHasEnemy ? FVector::Dist2D(
			OwnerAICharacter->GetActorLocation(), NearestEnemy->GetActorLocation()) : 0.0f;
		const bool bEnemyChanged = bHasEnemy && CompletedRetreatEnemy.Get() != NearestEnemy;
		const bool bEnemyCaughtUp = bHasEnemy && !bEnemyChanged &&
			CurrentEnemyDistance + RetreatClosingDistanceTolerance < CompletedRetreatEnemyDistance;
		if (!bHasEnemy || bEnemyChanged || bEnemyCaughtUp)
		{
			bRetreatCompletionBlocked = false;
			CompletedRetreatEnemy.Reset();
			CompletedRetreatEnemyDistance = 0.0f;
		}
	}

	if (!bRetreatInProgress)
	{
		return;
	}

	const bool bHasRetreatEnemy = bInShouldRetreat && InBattleSnapshot.bHasThreat && IsValid(NearestEnemy);
	if (bHasRetreatEnemy)
	{
		const float CurrentEnemyDistance = FVector::Dist2D(
			OwnerAICharacter->GetActorLocation(), NearestEnemy->GetActorLocation());
		const bool bEnemyChanged = LastRetreatEnemy.Get() != NearestEnemy;
		const bool bEnemyReacquired = !bHadRetreatEnemy;
		const bool bEnemyClosing = !bEnemyChanged && bHadRetreatEnemy &&
			CurrentEnemyDistance + RetreatClosingDistanceTolerance < LastRetreatEnemyDistance;

		// 敌方重新出现、目标切换或由远离转为追近时，从当前位置重新累计完整逃跑距离。
		if (bEnemyChanged || bEnemyReacquired || (bEnemyClosing && !bRetreatEnemyWasClosing))
		{
			RetreatStartLocation = OwnerAICharacter->GetActorLocation();
		}

		LastRetreatEnemy = NearestEnemy;
		LastRetreatEnemyDistance = CurrentEnemyDistance;
		bRetreatEnemyWasClosing = bEnemyClosing;
		bHadRetreatEnemy = true;

		const FVector UpdatedDirection = (OwnerAICharacter->GetActorLocation() -
			NearestEnemy->GetActorLocation()).GetSafeNormal2D();
		if (!UpdatedDirection.IsNearlyZero())
		{
			LastRetreatDirection = UpdatedDirection;
		}
	}
	else
	{
		bHadRetreatEnemy = false;
		bRetreatEnemyWasClosing = false;
	}

	const float RequiredRetreatDistance = ConvertMetersToWorldDistance(
		OwnerAICharacter->GetAIControlConfig().RetreatDistance);
	if (FVector::Dist2D(OwnerAICharacter->GetActorLocation(), RetreatStartLocation) >= RequiredRetreatDistance)
	{
		CompletedRetreatEnemy = IsValid(NearestEnemy) ? NearestEnemy : LastRetreatEnemy;
		CompletedRetreatEnemyDistance = CompletedRetreatEnemy.IsValid() ? FVector::Dist2D(
			OwnerAICharacter->GetActorLocation(), CompletedRetreatEnemy->GetActorLocation()) : 0.0f;
		if (CharacterBehaviorControlComponent)
		{
			CharacterBehaviorControlComponent->StopActiveMovement();
		}
		bRetreatInProgress = false;
		RetreatStartLocation = FVector::ZeroVector;
		LastRetreatDirection = FVector::ZeroVector;
		LastRetreatEnemy.Reset();
		LastRetreatEnemyDistance = 0.0f;
		bHadRetreatEnemy = false;
		bRetreatEnemyWasClosing = false;
		bRetreatCompletionBlocked = true;
	}
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecutePatrol()
{
	if (!CharacterBehaviorControlComponent || CharacterBehaviorControlComponent->IsNavigationMoving())
	{
		return CharacterBehaviorControlComponent ? ELxAIBehaviorExecutionResult::InProgress :
			ELxAIBehaviorExecutionResult::Failed;
	}

	FNavLocation PatrolLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		const float PatrolRadius = ConvertMetersToWorldDistance(OwnerAICharacter->GetAIControlConfig().PatrolRadius);
		if (NavigationSystem->GetRandomReachablePointInRadius(PatrolOrigin,
			PatrolRadius, PatrolLocation))
		{
			return CharacterBehaviorControlComponent->RequestMoveToLocation(PatrolLocation.Location, 50.0f) ?
				ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
		}
	}
	return ELxAIBehaviorExecutionResult::Failed;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorComponent::ExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	CharacterBehaviorControlComponent->StopActiveMovement();
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
	const float AttackSkillRange = ConvertMetersToWorldDistance(Config.AttackSkillRange);
	if (Distance > AttackSkillRange)
	{
		const float AcceptanceRadius = FMath::Min(
			ConvertMetersToWorldDistance(Config.AttackAcceptanceRadius), AttackSkillRange);
		return CharacterBehaviorControlComponent->RequestMoveToActor(TargetActor, AcceptanceRadius) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}

	CharacterBehaviorControlComponent->StopActiveMovement();
	ULxSkillBackpackModule* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	ULxSkillCastModule* SkillCast = OwnerAICharacter->GetSkillCastComponent();
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
		return CharacterBehaviorControlComponent->RequestMoveToLocation(InBattleSnapshot.AssistCenter, 150.0f) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}
	CharacterBehaviorControlComponent->StopActiveMovement();
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
	const float HealSkillRange = ConvertMetersToWorldDistance(Config.HealSkillRange);
	if (Distance > HealSkillRange)
	{
		return CharacterBehaviorControlComponent->RequestMoveToActor(HealTarget, HealSkillRange * 0.8f) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}

	CharacterBehaviorControlComponent->StopActiveMovement();
	ULxSkillBackpackModule* SkillBackpack = OwnerAICharacter->GetSkillBackpackComponent();
	ULxSkillCastModule* SkillCast = OwnerAICharacter->GetSkillCastComponent();
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
	if (!bRetreatInProgress && !IsValid(NearestEnemy))
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}

	const FVector CurrentLocation = OwnerAICharacter->GetActorLocation();
	if (!bRetreatInProgress)
	{
		bRetreatInProgress = true;
		RetreatStartLocation = CurrentLocation;
		LastRetreatEnemy = NearestEnemy;
		LastRetreatEnemyDistance = FVector::Dist2D(CurrentLocation, NearestEnemy->GetActorLocation());
		bHadRetreatEnemy = true;
		bRetreatEnemyWasClosing = false;
	}

	if (CharacterBehaviorControlComponent->IsNavigationMoving())
	{
		return ELxAIBehaviorExecutionResult::InProgress;
	}

	const FVector EnemyLocation = IsValid(NearestEnemy) ? NearestEnemy->GetActorLocation() :
		CurrentLocation - LastRetreatDirection * ConvertMetersToWorldDistance(RetreatMoveStepDistanceMeters);
	FVector RetreatDirection = IsValid(NearestEnemy) ?
		(CurrentLocation - EnemyLocation).GetSafeNormal2D() : LastRetreatDirection;
	if (RetreatDirection.IsNearlyZero())
	{
		RetreatDirection = -OwnerAICharacter->GetActorForwardVector();
	}
	LastRetreatDirection = RetreatDirection.GetSafeNormal2D();
	const float RetreatMoveStepDistance = ConvertMetersToWorldDistance(RetreatMoveStepDistanceMeters);

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
				if (!IsCompleteRetreatPath(NavigationPath))
				{
					continue;
				}

				if (AAIController* AIController = GetOwnerAIController())
				{
					AIController->ClearFocus(EAIFocusPriority::Gameplay);
				}
				if (CharacterBehaviorControlComponent->RequestMoveToLocation(ReachableLocation.Location, 35.0f))
				{
					return ELxAIBehaviorExecutionResult::Started;
				}
			}
		}
	}
	return ELxAIBehaviorExecutionResult::Failed;
}

void ULxAIBehaviorComponent::ResetRetreatState()
{
	bRetreatInProgress = false;
	RetreatStartLocation = FVector::ZeroVector;
	LastRetreatDirection = FVector::ZeroVector;
	LastRetreatEnemy.Reset();
	LastRetreatEnemyDistance = 0.0f;
	bHadRetreatEnemy = false;
	bRetreatEnemyWasClosing = false;
	bRetreatCompletionBlocked = false;
	CompletedRetreatEnemy.Reset();
	CompletedRetreatEnemyDistance = 0.0f;
}

AAIController* ULxAIBehaviorComponent::GetOwnerAIController() const
{
	return OwnerAICharacter ? Cast<AAIController>(OwnerAICharacter->GetController()) : nullptr;
}
