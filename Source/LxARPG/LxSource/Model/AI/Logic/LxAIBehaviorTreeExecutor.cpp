#include "LxAIBehaviorTreeExecutor.h"

#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterLifecycleAttributeObject.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/NavigationSystem/LxAINavigationRegistry.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIPointActor.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIRouteActor.h"

namespace
{
	constexpr float MetersToCentimeters = 100.0f;
	constexpr float DefaultAcceptanceRadius = 75.0f;
}

void ULxAIBehaviorTreeExecutor::Initialize(ALxAICharacter* InCharacter)
{
	ResetExecution();
	Character = InCharacter;
}

bool ULxAIBehaviorTreeExecutor::CanInterruptCurrentAction() const
{
	const ULxAIBehaviorTreeNodeData* Node = OrderedActions.IsValidIndex(CurrentActionIndex) ? OrderedActions[CurrentActionIndex] : nullptr;
	return !bCurrentLeafRunning || !Node || Node->bCanInterrupt;
}

bool ULxAIBehaviorTreeExecutor::TickExecution(const ULxAIBehaviorTreeAsset* InAsset,
	const FLxAIAnalysisDecision& InDecision, AActor* InKnownEnemy, const FVector& InLastKnownEnemyLocation,
	const float InDeltaSeconds)
{
	bReachedPatrolDestinationThisTick = false;
	if (!Character || !InAsset || !InDecision.bHasBranch)
	{
		ResetExecution();
		return false;
	}
	if (bDeathActionCompleted) return false;
	if (InDecision.Entry == ELxAIBehaviorEntry::CharacterDeath && !InDecision.PhaseId.IsValid())
	{
		if (ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent())
		{
			if (ULxCharacterLifecycleAttributeObject* Lifecycle = Attributes->GetLifecycleAttributeObject()) Lifecycle->SetDeferDeathForAI(false);
			Attributes->SetCharacterDead();
		}
		bDeathActionCompleted = true;
		return false;
	}
	if (InDecision.Entry == ELxAIBehaviorEntry::CharacterDeath && (CurrentPhaseId != InDecision.PhaseId || InDecision.bStartNewBehavior))
	{
		if (!EnterPhase(InAsset, InDecision))
		{
			if (ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent())
			{
				if (ULxCharacterLifecycleAttributeObject* Lifecycle = Attributes->GetLifecycleAttributeObject()) Lifecycle->SetDeferDeathForAI(false);
				Attributes->SetCharacterDead();
			}
			bDeathActionCompleted = true;
			return false;
		}
	}
	if (InDecision.Entry == ELxAIBehaviorEntry::CharacterDeath)
	{
		const ULxAIBehaviorTreeNodeData* DeathAction = OrderedActions.IsValidIndex(CurrentActionIndex) ? OrderedActions[CurrentActionIndex] : nullptr;
		if (!DeathAction || DeathAction->Action != ELxAIBehaviorAction::EnterDeath)
			for (ULxAIBehaviorTreeNodeData* Candidate : OrderedActions)
				if (Candidate && Candidate->Action == ELxAIBehaviorAction::EnterDeath) { DeathAction = Candidate; break; }
		if (DeathAction)
		{
			ChangeLeaf(DeathAction);
			TickLeaf(*DeathAction, InKnownEnemy, InLastKnownEnemyLocation, InDeltaSeconds);
			bDeathActionCompleted = true;
			if (ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent())
			{
				if (ULxCharacterLifecycleAttributeObject* Lifecycle = Attributes->GetLifecycleAttributeObject()) Lifecycle->SetDeferDeathForAI(false);
				Attributes->SetCharacterDead();
			}
			ChangeLeaf(nullptr);
			return false;
		}
		if (ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent())
		{
			if (ULxCharacterLifecycleAttributeObject* Lifecycle = Attributes->GetLifecycleAttributeObject()) Lifecycle->SetDeferDeathForAI(false);
			Attributes->SetCharacterDead();
		}
		bDeathActionCompleted = true;
		return false;
	}
	if (bRouteFleeCompleted && CurrentPhaseId == InDecision.PhaseId)
	{
		// 路线完成仅阻止整阶段重跑，后续随机逃跑等叶节点仍须逐帧执行。
		if (InDecision.Entry == ELxAIBehaviorEntry::Attacked && InDecision.bStartNewBehavior)
			bRouteFleeCompleted = false;
		else if (!OrderedActions.IsValidIndex(CurrentActionIndex))
			return InDecision.Entry == ELxAIBehaviorEntry::Attacked;
	}
	if (CurrentPhaseId != InDecision.PhaseId) bRouteFleeCompleted = false;
	if (CurrentPhaseId != InDecision.PhaseId || InDecision.bStartNewBehavior)
	{
		if (!EnterPhase(InAsset, InDecision))
		{
			return false;
		}
	}
	if (!OrderedActions.IsValidIndex(CurrentActionIndex)) return true;

	// 失败时在本轮立即尝试后续行为；索引只向后推进，避免所有行为失败时在同帧死循环。
	while (OrderedActions.IsValidIndex(CurrentActionIndex))
	{
		const ULxAIBehaviorTreeNodeData* Node = OrderedActions[CurrentActionIndex];
		ChangeLeaf(Node);
		const ELeafResult Result = Node ? TickLeaf(*Node, InKnownEnemy, InLastKnownEnemyLocation, InDeltaSeconds) : ELeafResult::Failed;
		bCurrentLeafRunning = Result == ELeafResult::Running;
		if (Result == ELeafResult::Running) return false;

		ResetLeafProgress(Result == ELeafResult::Failed);
		++CurrentActionIndex;
		if (Result == ELeafResult::Completed && OrderedActions.IsValidIndex(CurrentActionIndex))
		{
			ChangeLeaf(OrderedActions[CurrentActionIndex]);
			return false;
		}
	}
	if (InDecision.Entry == ELxAIBehaviorEntry::Attacked)
	{
		ChangeLeaf(nullptr);
		return true;
	}
	if (bRouteFleeCompleted)
	{
		ChangeLeaf(nullptr);
		return false;
	}
	// 持续入口执行完一轮后从首叶重新开始，使技能和等待行为可以持续工作。
	CurrentActionIndex = 0;
	LeafStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	WaitStartTime = -1.0;
	ChangeLeaf(OrderedActions.IsValidIndex(0) ? OrderedActions[0] : nullptr);
	return false;
}

void ULxAIBehaviorTreeExecutor::ResetExecution()
{
	ResetLeafProgress(true);
	OrderedActions.Reset();
	CurrentPhaseId.Invalidate();
	bRouteFleeCompleted = false;
	bDeathActionCompleted = false;
	CurrentActionIndex = INDEX_NONE;
	ChangeLeaf(nullptr);
}

bool ULxAIBehaviorTreeExecutor::EnterPhase(const ULxAIBehaviorTreeAsset* InAsset,
	const FLxAIAnalysisDecision& InDecision)
{
	ResetLeafProgress(true);
	OrderedActions.Reset();
	const ULxAIBehaviorTreeNodeData* Phase = InAsset->FindNode(InDecision.PhaseId);
	if (!Phase || Phase->Kind != ELxAIBehaviorNodeKind::Phase) return false;
	for (const FGuid& ChildId : Phase->Children)
	{
		ULxAIBehaviorTreeNodeData* Child = InAsset->FindNode(ChildId);
		if (Child && Child->Kind == ELxAIBehaviorNodeKind::Action) OrderedActions.Add(Child);
	}
	OrderedActions.StableSort([](const ULxAIBehaviorTreeNodeData& Left, const ULxAIBehaviorTreeNodeData& Right)
	{
		return Left.Order < Right.Order;
	});
	CurrentPhaseId = InDecision.PhaseId;
	CurrentActionIndex = OrderedActions.IsEmpty() ? INDEX_NONE : 0;
	ChangeLeaf(OrderedActions.IsValidIndex(CurrentActionIndex) ? OrderedActions[CurrentActionIndex] : nullptr);
	return !OrderedActions.IsEmpty();
}

ULxAIBehaviorTreeExecutor::ELeafResult ULxAIBehaviorTreeExecutor::TickLeaf(
	const ULxAIBehaviorTreeNodeData& InNode, AActor* InEnemy, const FVector& InEnemyLocation, const float InDeltaSeconds)
{
	ULxCharacterBehaviorControlComponent* Behavior = Character->GetCharacterBehaviorControlComponent();
	if (!Behavior) return ELeafResult::Failed;
	const FVector SelfLocation = Character->GetActorLocation();
	const float EnemyDistance = FVector::Dist2D(SelfLocation, InEnemyLocation);
	const bool bFaceEnemy = IsValid(InEnemy) && (InNode.Action == ELxAIBehaviorAction::Alert ||
		InNode.Action == ELxAIBehaviorAction::Defend || InNode.Action == ELxAIBehaviorAction::MeleeSkill ||
		InNode.Action == ELxAIBehaviorAction::RangedSkill);
	if (bFaceEnemy)
	{
		Behavior->SetDesiredFacingDirection(InEnemyLocation - SelfLocation);
		if (!bHasBehaviorFacingRequest)
		{
			Behavior->AddFacingControlRequest();
			bHasBehaviorFacingRequest = true;
		}
	}
	else if (bHasBehaviorFacingRequest)
	{
		Behavior->RemoveFacingControlRequest(0.0f);
		bHasBehaviorFacingRequest = false;
	}

	switch (InNode.Action)
	{
	case ELxAIBehaviorAction::Wait:
		if (InNode.WaitSeconds <= 0.0f) return ELeafResult::Running;
		return GetWorld() && GetWorld()->GetTimeSeconds() - LeafStartTime >= InNode.WaitSeconds ? ELeafResult::Completed : ELeafResult::Running;
	case ELxAIBehaviorAction::EnterDeath:
		if (ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent()) Attributes->SetCharacterDead();
		return ELeafResult::Completed;
	case ELxAIBehaviorAction::PointPatrol:
	case ELxAIBehaviorAction::PointFlee:
	{
		ULxAINavigationRegistry* Registry = GetNavigationRegistry();
		ALxAIPointActor* Point = Registry ? Registry->FindPoint(this, InNode.PointId) : nullptr;
		if (!Point)
		{
			if (!bLoggedNavigationFailure)
				UE_LOG(LogTemp, Warning, TEXT("AI点位尚未加载：角色=%s，点位=%s"),
					*Character->GetName(), *InNode.PointId.ToString());
			bLoggedNavigationFailure = true;
			return ELeafResult::Failed;
		}
		if (InNode.Action == ELxAIBehaviorAction::PointFlee)
		{
			bLoggedNavigationFailure = false;
			const float PointRadius = Point->GetRangeRadiusCentimeters();
			if (FVector::Dist2D(SelfLocation, Point->GetWorldCenter()) <= PointRadius) return ELeafResult::Completed;
			return Behavior->IsNavigationMoving() || MoveToLocation(Point->GetWorldCenter(), PointRadius)
				? ELeafResult::Running : ELeafResult::Failed;
		}
		if (!bHasPointPatrolDestination && !ChoosePointPatrolDestination(*Point, SelfLocation))
		{
			if (!bLoggedNavigationFailure)
				UE_LOG(LogTemp, Warning, TEXT("AI巡逻点范围内暂无可到达位置：角色=%s，点位=%s"),
					*Character->GetName(), *InNode.PointId.ToString());
			bLoggedNavigationFailure = true;
			return ELeafResult::Failed;
		}
		bLoggedNavigationFailure = false;
		if (FVector::Dist2D(SelfLocation, PointPatrolDestination) > DefaultAcceptanceRadius)
			return Behavior->IsNavigationMoving() || MoveToLocation(PointPatrolDestination, DefaultAcceptanceRadius)
				? ELeafResult::Running : ELeafResult::Failed;
		Behavior->StopActiveMovement();
		bReachedPatrolDestinationThisTick = true;
		if (InNode.WaitSeconds <= 0.0f) return ELeafResult::Completed;
		if (WaitStartTime < 0.0) WaitStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		return GetWorld() && GetWorld()->GetTimeSeconds() - WaitStartTime >= InNode.WaitSeconds ? ELeafResult::Completed : ELeafResult::Running;
	}
	case ELxAIBehaviorAction::RoutePatrol:
	case ELxAIBehaviorAction::RouteFlee:
	{
		if (CachedRoutePoints.IsEmpty())
		{
			if (!ActiveRoute.IsValid())
			{
				ULxAINavigationRegistry* Registry = GetNavigationRegistry();
				ActiveRoute = Registry ? Registry->FindRoute(this, InNode.RouteId) : nullptr;
			}
			if (ActiveRoute.IsValid())
			{
				CachedRoutePoints = ActiveRoute->GetWorldRoutePoints();
				if (InNode.Action == ELxAIBehaviorAction::RouteFlee && !CachedRoutePoints.IsEmpty())
				{
					float NearestDistanceSquared = TNumericLimits<float>::Max();
					for (int32 PointIndex = 0; PointIndex < CachedRoutePoints.Num(); ++PointIndex)
					{
						const float DistanceSquared = FVector::DistSquared2D(SelfLocation, CachedRoutePoints[PointIndex]);
						if (DistanceSquared < NearestDistanceSquared)
						{
							NearestDistanceSquared = DistanceSquared;
							RoutePointIndex = PointIndex;
						}
					}
				}
			}
		}
		const TArray<FVector>& Points = CachedRoutePoints;
		if (Points.IsEmpty())
		{
			if (!bLoggedNavigationFailure)
				UE_LOG(LogTemp, Warning, TEXT("AI路线尚未加载：角色=%s，路线=%s"),
					*Character->GetName(), *InNode.RouteId.ToString());
			bLoggedNavigationFailure = true;
			return ELeafResult::Failed;
		}
		if (!Points.IsValidIndex(RoutePointIndex)) RoutePointIndex = 0;
		if (InNode.Action == ELxAIBehaviorAction::RouteFlee && !bHasRouteFleeDestination)
			ChooseRouteFleeDestination(Points[RoutePointIndex], InNode.RouteFleeDeviationMeters);
		const FVector& Destination = InNode.Action == ELxAIBehaviorAction::RouteFlee ? RouteFleeDestination : Points[RoutePointIndex];
		if (FVector::Dist2D(SelfLocation, Destination) <= DefaultAcceptanceRadius)
		{
			if (InNode.Action == ELxAIBehaviorAction::RoutePatrol) bReachedPatrolDestinationThisTick = true;
			bLoggedNavigationFailure = false;
			if (InNode.Action == ELxAIBehaviorAction::RouteFlee && InNode.bRouteFleeUseOnce && RoutePointIndex == Points.Num() - 1)
			{
				Behavior->StopActiveMovement();
				bRouteFleeCompleted = true;
				return ELeafResult::Completed;
			}
			if (InNode.Action == ELxAIBehaviorAction::RoutePatrol && InNode.WaitSeconds > 0.0f)
			{
				if (WaitStartTime < 0.0) WaitStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
				if (!GetWorld() || GetWorld()->GetTimeSeconds() - WaitStartTime < InNode.WaitSeconds) return ELeafResult::Running;
			}
			WaitStartTime = -1.0;
			Behavior->StopActiveMovement();
			RoutePointIndex += RouteDirection;
			bHasRouteFleeDestination = false;
			if (RoutePointIndex >= Points.Num() || RoutePointIndex < 0)
			{
				if (InNode.Action == ELxAIBehaviorAction::RouteFlee || InNode.RouteMode == ELxAIRoutePatrolMode::Loop) RoutePointIndex = 0;
				else { RouteDirection *= -1; RoutePointIndex = FMath::Clamp(RoutePointIndex + 2 * RouteDirection, 0, Points.Num() - 1); }
			}
			if (InNode.Action == ELxAIBehaviorAction::RouteFlee)
				ChooseRouteFleeDestination(Points[RoutePointIndex], InNode.RouteFleeDeviationMeters);
		}
		if (Behavior->IsNavigationMoving() || Behavior->RequestMoveToLocationDirect(
			InNode.Action == ELxAIBehaviorAction::RouteFlee ? RouteFleeDestination : Points[RoutePointIndex], DefaultAcceptanceRadius))
		{
			bLoggedNavigationFailure = false;
			return ELeafResult::Running;
		}
		if (!bLoggedNavigationFailure)
			UE_LOG(LogTemp, Warning, TEXT("AI路线移动请求失败：角色=%s，路线=%s，路径点=%d"),
				*Character->GetName(), *InNode.RouteId.ToString(), RoutePointIndex);
		bLoggedNavigationFailure = true;
		return ELeafResult::Failed;
	}
	case ELxAIBehaviorAction::MeleeSkill:
		if (!IsValid(InEnemy)) return ELeafResult::Failed;
		if (EnemyDistance > InNode.MeleeDistanceMeters * MetersToCentimeters)
			return Behavior->RequestMoveToActor(InEnemy, InNode.MeleeDistanceMeters * MetersToCentimeters * 0.8f) || Behavior->IsNavigationMoving() ? ELeafResult::Running : ELeafResult::Failed;
		Behavior->StopActiveMovement();
		return ReleaseSkill(InNode, InEnemy, InEnemyLocation);
	case ELxAIBehaviorAction::RangedSkill:
		if (!IsValid(InEnemy)) return ELeafResult::Failed;
		if (EnemyDistance > InNode.MaxDistanceMeters * MetersToCentimeters)
			return Behavior->RequestMoveToActor(InEnemy, InNode.MaxDistanceMeters * MetersToCentimeters * 0.9f) || Behavior->IsNavigationMoving() ? ELeafResult::Running : ELeafResult::Failed;
		if (EnemyDistance < InNode.MinDistanceMeters * MetersToCentimeters)
		{
			const FVector AwayDirection = (SelfLocation - InEnemyLocation).GetSafeNormal2D();
			return !AwayDirection.IsNearlyZero() && MoveToLocation(
				SelfLocation + AwayDirection * InNode.MinDistanceMeters * MetersToCentimeters,
				DefaultAcceptanceRadius) ? ELeafResult::Running : ELeafResult::Failed;
		}
		Behavior->StopActiveMovement();
		return ReleaseSkill(InNode, InEnemy, InEnemyLocation);
	case ELxAIBehaviorAction::BuffSkill:
		return ReleaseSkill(InNode, Character, SelfLocation);
	case ELxAIBehaviorAction::Alert:
	case ELxAIBehaviorAction::Defend:
		if (!IsValid(InEnemy)) return ELeafResult::Failed;
		if (EnemyDistance > InNode.MaxDistanceMeters * MetersToCentimeters)
			return Behavior->RequestMoveToActor(InEnemy, InNode.MaxDistanceMeters * MetersToCentimeters)
				? ELeafResult::Running : ELeafResult::Failed;
		else if (EnemyDistance < InNode.MinDistanceMeters * MetersToCentimeters)
			return MoveToLocation(SelfLocation + (SelfLocation - InEnemyLocation).GetSafeNormal2D() * InNode.MinDistanceMeters * MetersToCentimeters, DefaultAcceptanceRadius)
				? ELeafResult::Running : ELeafResult::Failed;
		else Behavior->StopActiveMovement();
		return ELeafResult::Running;
	case ELxAIBehaviorAction::RandomFlee:
	{
		if (!IsValid(InEnemy)) return ELeafResult::Failed;
		if (EnemyDistance >= InNode.FleeSafeDistanceMeters * MetersToCentimeters)
		{
			Behavior->StopActiveMovement();
			return ELeafResult::Completed;
		}
		if (bHasRandomFleeDestination && Behavior->IsNavigationMoving())
		{
			if (FVector::Dist2D(SelfLocation, RandomFleeProgressLocation) >= 10.0f)
			{
				RandomFleeProgressLocation = SelfLocation;
				RandomFleeStalledSeconds = 0.0f;
			}
			else RandomFleeStalledSeconds += InDeltaSeconds;
			if (RandomFleeStalledSeconds < 0.5f && IsRandomFleeSegmentClear(RandomFleeDestination))
				return ELeafResult::Running;
		}
		// 障碍出现、移动失败或短时间无进展时，停止旧请求并在同轮重选其他方向。
		Behavior->StopActiveMovement();
		if (ChooseRandomFleeDestination(InNode, InEnemyLocation)) return ELeafResult::Running;
		if (!bLoggedNavigationFailure)
			UE_LOG(LogTemp, Warning, TEXT("AI随机逃跑各方向均无可用目标，切换后续行为：角色=%s，位置=%s"),
				*Character->GetName(), *SelfLocation.ToString());
		bLoggedNavigationFailure = true;
		return ELeafResult::Failed;
	}
	default:
		return ELeafResult::Failed;
	}
}

void ULxAIBehaviorTreeExecutor::ChangeLeaf(const ULxAIBehaviorTreeNodeData* InNode)
{
	if (Character && Character->GetCharacterBehaviorControlComponent())
	{
		const ELxCharacterMotionType MotionType = InNode ? InNode->GetMotionType() : ELxCharacterMotionType::None;
		Character->GetCharacterBehaviorControlComponent()->SetBehaviorMotion(MotionType);
	}
	const FGuid NewId = InNode ? InNode->NodeId : FGuid();
	const ELxAIBehaviorAction NewAction = InNode ? InNode->Action : ELxAIBehaviorAction::Wait;
	if (CurrentActionNodeId == NewId) return;
	bCurrentLeafRunning = false;
	CurrentActionNodeId = NewId;
	CurrentAction = NewAction;
	LeafStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	OnLeafChanged.ExecuteIfBound(CurrentAction);
}

void ULxAIBehaviorTreeExecutor::ResetLeafProgress(const bool bStopMovement)
{
	if (bHasBehaviorFacingRequest && Character && Character->GetCharacterBehaviorControlComponent())
		Character->GetCharacterBehaviorControlComponent()->RemoveFacingControlRequest(0.0f);
	bHasBehaviorFacingRequest = false;
	bCurrentLeafRunning = false;
	WaitStartTime = -1.0;
	ActiveRoute.Reset();
	CachedRoutePoints.Reset();
	PointPatrolDestination = FVector::ZeroVector;
	bHasPointPatrolDestination = false;
	bLoggedNavigationFailure = false;
	RoutePointIndex = 0;
	RouteDirection = 1;
	RouteFleeDestination = FVector::ZeroVector;
	bHasRouteFleeDestination = false;
	RandomFleeDestination = FVector::ZeroVector;
	bHasRandomFleeDestination = false;
	RandomFleeProgressLocation = FVector::ZeroVector;
	RandomFleeStalledSeconds = 0.0f;
	if (bStopMovement && Character && Character->GetCharacterBehaviorControlComponent())
		Character->GetCharacterBehaviorControlComponent()->StopActiveMovement();
}

bool ULxAIBehaviorTreeExecutor::MoveToLocation(const FVector& InLocation, const float InAcceptanceRadius)
{
	ULxCharacterBehaviorControlComponent* Behavior = Character ? Character->GetCharacterBehaviorControlComponent() : nullptr;
	return Behavior && Behavior->RequestMoveToLocation(InLocation, FMath::Max(1.0f, InAcceptanceRadius));
}

bool ULxAIBehaviorTreeExecutor::ChoosePointPatrolDestination(const ALxAIPointActor& InPoint,
	const FVector& InSelfLocation)
{
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation) return false;
	const float Radius = InPoint.GetRangeRadiusCentimeters();
	if (Radius <= DefaultAcceptanceRadius) return false;
	FNavLocation SearchOrigin;
	if (!Navigation->ProjectPointToNavigation(InPoint.GetWorldCenter(), SearchOrigin,
		FVector(Radius, Radius, FMath::Max(500.0f, Radius)))) return false;
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		FNavLocation Candidate;
		if (Navigation->GetRandomReachablePointInRadius(SearchOrigin.Location, Radius, Candidate) &&
			InPoint.IsWorldLocationInRange(Candidate.Location) &&
			FVector::Dist2D(InSelfLocation, Candidate.Location) > DefaultAcceptanceRadius * 2.0f)
		{
			PointPatrolDestination = Candidate.Location;
			bHasPointPatrolDestination = true;
			return true;
		}
	}
	return false;
}

void ULxAIBehaviorTreeExecutor::ChooseRouteFleeDestination(const FVector& InRoutePoint, const float InDeviationMeters)
{
	RouteFleeDestination = InRoutePoint;
	if (InDeviationMeters > 0.0f)
	{
		if (UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld()))
		{
			FNavLocation RouteNavPoint;
			FNavLocation RandomNavPoint;
			if (Navigation->ProjectPointToNavigation(InRoutePoint, RouteNavPoint) &&
				Navigation->GetRandomReachablePointInRadius(RouteNavPoint.Location,
					InDeviationMeters * MetersToCentimeters, RandomNavPoint) &&
				FVector::DistSquared2D(InRoutePoint, RandomNavPoint.Location) <= FMath::Square(InDeviationMeters * MetersToCentimeters))
				RouteFleeDestination = RandomNavPoint.Location;
		}
	}
	bHasRouteFleeDestination = true;
}

bool ULxAIBehaviorTreeExecutor::IsRandomFleeSegmentClear(const FVector& InDestination) const
{
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation || !Character) return false;
	FNavLocation Start;
	if (!Navigation->ProjectPointToNavigation(Character->GetActorLocation(), Start)) return false;
	FVector HitLocation;
	if (UNavigationSystemV1::NavigationRaycast(Character, Start.Location, InDestination, HitLocation, nullptr, Character->GetController())) return false;
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const FVector HeightOffset(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight());
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RandomFleeObstacle), false, Character);
	return !GetWorld()->SweepSingleByChannel(Hit, Start.Location + HeightOffset, InDestination + HeightOffset,
		FQuat::Identity, Capsule->GetCollisionObjectType(), FCollisionShape::MakeSphere(Radius), Params,
		FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()));
}

bool ULxAIBehaviorTreeExecutor::ChooseRandomFleeDestination(const ULxAIBehaviorTreeNodeData& InNode, const FVector& InEnemyLocation)
{
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation) return false;
	const FVector SelfLocation = Character->GetActorLocation();
	FVector Away = (SelfLocation - InEnemyLocation).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = -Character->GetActorForwardVector();
	// 先搜索远离敌人的扇区，再尝试两侧和背向；每个方向同时尝试短距离目标。
	const float Side = FMath::RandBool() ? 1.0f : -1.0f;
	const float Angles[] = {0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 90.0f, -90.0f, 120.0f, -120.0f, 150.0f, -150.0f, 180.0f};
	for (const float Angle : Angles)
	{
		const FVector Direction = Away.RotateAngleAxis(Angle * Side + FMath::FRandRange(-10.0f, 10.0f), FVector::UpVector);
		for (const float Scale : {1.0f, 0.5f, 0.25f})
		{
			FNavLocation Candidate;
			if (!Navigation->ProjectPointToNavigation(SelfLocation + Direction * InNode.FleeStepMeters * MetersToCentimeters * Scale, Candidate)
				|| FVector::Dist2D(SelfLocation, Candidate.Location) <= DefaultAcceptanceRadius * 2.0f
				|| (bHasRandomFleeDestination && FVector::Dist2D(RandomFleeDestination, Candidate.Location) <= DefaultAcceptanceRadius * 2.0f)
				|| !IsRandomFleeSegmentClear(Candidate.Location)) continue;
			if (!Character->GetCharacterBehaviorControlComponent()->RequestMoveToLocationDirect(Candidate.Location, DefaultAcceptanceRadius)) continue;
			RandomFleeDestination = Candidate.Location;
			bHasRandomFleeDestination = true;
			RandomFleeProgressLocation = SelfLocation;
			RandomFleeStalledSeconds = 0.0f;
			bLoggedNavigationFailure = false;
			return true;
		}
	}
	return false;
}

ULxAINavigationRegistry* ULxAIBehaviorTreeExecutor::GetNavigationRegistry() const
{
	const ULxGameInstanceSubsystem* Subsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	return Subsystem ? Subsystem->GetAINavigationRegistry() : nullptr;
}

ULxAIBehaviorTreeExecutor::ELeafResult ULxAIBehaviorTreeExecutor::ReleaseSkill(
	const ULxAIBehaviorTreeNodeData& InNode, AActor* InTarget, const FVector& InTargetLocation)
{
	ULxSkillBackpackModule* Backpack = Character->GetSkillBackpackComponent();
	ULxSkillCastModule* Cast = Character->GetSkillCastComponent();
	if (!Backpack || !Cast) return ELeafResult::Failed;
	if (!Cast->IsSkillCastIdle()) return ELeafResult::Running;
	ULxSkillItem* Item = Backpack->FindSkillItemByTagID(InNode.SkillItemId);
	if (!Item)
	{
		Backpack->AddSkillItemsByTagID({InNode.SkillItemId});
		Item = Backpack->FindSkillItemByTagID(InNode.SkillItemId);
	}
	if (!Item) return ELeafResult::Failed;
	const FVector Direction = (InTargetLocation - Character->GetActorLocation()).GetSafeNormal();
	const FLxSkillCastContext Context = Cast->MakeSkillCastContext(Character, InTarget, InTargetLocation, true, Direction, !Direction.IsNearlyZero());
	return Cast->ReleaseSkillItemDirectly(Item, Context) ? ELeafResult::Completed : ELeafResult::Failed;
}
