#include "LxAIBehaviorModule.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

namespace
{
	/** 将AI行为配置使用的米换算为虚幻世界单位厘米。 */
	constexpr float AIBehaviorModuleMetersToCentimeters = 100.0f;

	/** 将非负的AI业务距离从米换算为虚幻世界单位厘米。 */
	float ConvertModuleMetersToWorldDistance(const float InDistanceMeters)
	{
		return FMath::Max(0.0f, InDistanceMeters) * AIBehaviorModuleMetersToCentimeters;
	}

	/** 长距离逃跑目标不可达时保留的最小单次移动距离，配置语义为米。 */
	constexpr float ModuleMinimumRetreatMoveDistanceMeters = 3.0f;

	/** 逃跑目标点投射失败时逐级缩短距离的尝试比例。 */
	constexpr float ModuleRetreatDistanceScales[] = {1.0f, 0.75f, 0.5f, 0.25f, 0.125f, 0.0625f};

	/** 主逃跑方向不可导航时在后方扇区内按左右交替顺序尝试的水平偏转角度。 */
	constexpr float ModuleRetreatDirectionAngles[] = {0.0f, 25.0f, -25.0f, 50.0f, -50.0f, 70.0f, -70.0f};

	/** 判断敌方开始追近时允许的距离抖动误差，单位为厘米。 */
	constexpr float ModuleRetreatClosingDistanceTolerance = 10.0f;

	/** 判断逃跑候选路径是否完整可达，允许导航系统为了绕过障碍短暂横移或接近敌方。 */
	bool IsCompleteModuleRetreatPath(const UNavigationPath* InNavigationPath)
	{
		return IsValid(InNavigationPath) && InNavigationPath->IsValid() && !InNavigationPath->IsPartial() &&
			InNavigationPath->PathPoints.Num() >= 2;
	}

	/** 获取角色脚底的导航代理位置，避免整体缩放后继续使用胶囊体中心查询地面导航。 */
	FVector GetModuleNavigationAgentLocation(const ALxAICharacter* InCharacter)
	{
		return IsValid(InCharacter) ? InCharacter->GetNavAgentLocation() : FVector::ZeroVector;
	}

	/** 根据缩放后的胶囊体与跨步高度生成导航点投射范围。 */
	FVector GetModuleNavigationProjectionExtent(const ALxAICharacter* InCharacter,
		const ANavigationData* InNavigationData)
	{
		const FVector DefaultExtent = IsValid(InNavigationData) ? InNavigationData->GetDefaultQueryExtent() :
			FVector(50.0f, 50.0f, 250.0f);
		float AgentRadius = 0.0f;
		float AgentHalfHeight = 0.0f;
		if (IsValid(InCharacter))
		{
			InCharacter->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);
		}
		const UCharacterMovementComponent* MovementComponent = IsValid(InCharacter) ?
			InCharacter->GetCharacterMovement() : nullptr;
		const float StepHeight = MovementComponent ? MovementComponent->MaxStepHeight : 0.0f;
		const double HorizontalExtent = FMath::Max3(DefaultExtent.X, DefaultExtent.Y,
			static_cast<double>(AgentRadius));
		const double VerticalExtent = FMath::Max(DefaultExtent.Z,
			static_cast<double>(AgentRadius + StepHeight + 50.0f));
		return FVector(HorizontalExtent, HorizontalExtent, VerticalExtent);
	}
}

void ULxAIBehaviorModule::InitializeModule(ULxAIControlComponent* InOwnerComponent)
{
	Super::InitializeModule(InOwnerComponent);
	OwnerAICharacter = Cast<ALxAICharacter>(GetOwner());
	CharacterBehaviorControlComponent = OwnerAICharacter ? OwnerAICharacter->GetCharacterBehaviorControlComponent() : nullptr;
	if (OwnerAICharacter)
	{
		PatrolOrigin = GetModuleNavigationAgentLocation(OwnerAICharacter);
	}
	bPatrolOriginNeedsRefresh = false;
}

void ULxAIBehaviorModule::ShutdownModule()
{
	StopBehavior();
	CharacterBehaviorControlComponent = nullptr;
	OwnerAICharacter = nullptr;
	Super::ShutdownModule();
}

bool ULxAIBehaviorModule::CanExecuteBehavior(const ELxAIActionType InActionType,
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

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecuteBehavior(const ELxAIActionType InActionType,
	const FLxAIBattleSnapshot& InBattleSnapshot)
{
	if (!OwnerAICharacter || !CharacterBehaviorControlComponent)
	{
		InitializeModule(OwnerComponent);
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

bool ULxAIBehaviorModule::CanExecutePatrol(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent && !InBattleSnapshot.bHasThreat;
}

bool ULxAIBehaviorModule::CanExecuteAlert(const FLxAIBattleSnapshot&) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent;
}

bool ULxAIBehaviorModule::CanExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot) const
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

bool ULxAIBehaviorModule::CanExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent && InBattleSnapshot.bHasThreat;
}

bool ULxAIBehaviorModule::CanExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot) const
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

bool ULxAIBehaviorModule::CanExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot) const
{
	return OwnerAICharacter && CharacterBehaviorControlComponent && (bRetreatInProgress ||
		(!bRetreatCompletionBlocked && InBattleSnapshot.bHasThreat && IsValid(InBattleSnapshot.NearestEnemy)));
}

void ULxAIBehaviorModule::StopBehavior()
{
	StopBehaviorExecution();
	ResetRetreatState();
}

void ULxAIBehaviorModule::StopBehaviorExecution()
{
	if (CharacterBehaviorControlComponent)
	{
		CharacterBehaviorControlComponent->StopActiveMovement();
	}
	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void ULxAIBehaviorModule::UpdateRetreatProgress(const FLxAIBattleSnapshot& InBattleSnapshot)
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
			CurrentEnemyDistance + ModuleRetreatClosingDistanceTolerance < CompletedRetreatEnemyDistance;
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

	const FVector CurrentLocation = OwnerAICharacter->GetActorLocation();
	AccumulatedRetreatDistance += FVector::Dist2D(CurrentLocation, LastRetreatSampleLocation);
	LastRetreatSampleLocation = CurrentLocation;

	const bool bHasRetreatEnemy = InBattleSnapshot.bHasThreat && IsValid(NearestEnemy);
	if (bHasRetreatEnemy)
	{
		const float CurrentEnemyDistance = FVector::Dist2D(
			CurrentLocation, NearestEnemy->GetActorLocation());
		const bool bEnemyChanged = LastRetreatEnemy.Get() != NearestEnemy;
		const bool bEnemyReacquired = !bHadRetreatEnemy;
		const bool bEnemyClosing = !bEnemyChanged && bHadRetreatEnemy &&
			CurrentEnemyDistance + ModuleRetreatClosingDistanceTolerance < LastRetreatEnemyDistance;

		// 敌方重新出现、目标切换或由远离转为追近时，从当前位置重新累计完整逃跑距离。
		if (bEnemyChanged || bEnemyReacquired || (bEnemyClosing && !bRetreatEnemyWasClosing))
		{
			RetreatStartLocation = CurrentLocation;
			LastRetreatSampleLocation = CurrentLocation;
			AccumulatedRetreatDistance = 0.0f;
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

	const float RequiredRetreatDistance = ConvertModuleMetersToWorldDistance(
		OwnerAICharacter->GetAIControlConfig().RetreatDistance);
	if (AccumulatedRetreatDistance >= RequiredRetreatDistance)
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
		LastRetreatSampleLocation = FVector::ZeroVector;
		AccumulatedRetreatDistance = 0.0f;
		LastRetreatDirection = FVector::ZeroVector;
		LastRetreatEnemy.Reset();
		LastRetreatEnemyDistance = 0.0f;
		bHadRetreatEnemy = false;
		bRetreatEnemyWasClosing = false;
		bRetreatCompletionBlocked = true;
	}
}

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecutePatrol()
{
	if (!CharacterBehaviorControlComponent || CharacterBehaviorControlComponent->IsNavigationMoving())
	{
		return CharacterBehaviorControlComponent ? ELxAIBehaviorExecutionResult::InProgress :
			ELxAIBehaviorExecutionResult::Failed;
	}
	if (bPatrolOriginNeedsRefresh && OwnerAICharacter)
	{
		// 逃跑后的生存位置成为新据点，避免巡逻再把角色拉回初始出生点。
		PatrolOrigin = GetModuleNavigationAgentLocation(OwnerAICharacter);
		bPatrolOriginNeedsRefresh = false;
	}

	FNavLocation NavigationPatrolOrigin;
	FNavLocation PatrolLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		const float PatrolRadius = ConvertModuleMetersToWorldDistance(OwnerAICharacter->GetAIControlConfig().PatrolRadius);
		const FNavAgentProperties& AgentProperties = OwnerAICharacter->GetNavAgentPropertiesRef();
		ANavigationData* NavigationData = NavigationSystem->GetNavDataForProps(AgentProperties, PatrolOrigin);
		const FVector QueryExtent = GetModuleNavigationProjectionExtent(OwnerAICharacter, NavigationData);
		// 使用脚底位置和当前体型对应的导航数据，防止巨大角色从胶囊中心误投射到其他楼层或默认代理网格。
		if (NavigationData && NavigationSystem->ProjectPointToNavigation(
			PatrolOrigin, NavigationPatrolOrigin, QueryExtent, NavigationData) &&
			NavigationSystem->GetRandomReachablePointInRadius(NavigationPatrolOrigin.Location,
			PatrolRadius, PatrolLocation, NavigationData))
		{
			if (CharacterBehaviorControlComponent->RequestMoveToLocation(PatrolLocation.Location, 50.0f))
			{
				UE_LOG(LogTemp, Verbose, TEXT("AI巡逻开始：角色=%s，目标=%s。"),
					*GetNameSafe(OwnerAICharacter), *PatrolLocation.Location.ToCompactString());
				return ELxAIBehaviorExecutionResult::Started;
			}
		}
	}
	UE_LOG(LogTemp, Verbose, TEXT("AI巡逻暂无可达点，等待重试：角色=%s，原点=%s。"),
		*GetNameSafe(OwnerAICharacter), *PatrolOrigin.ToCompactString());
	// 随机巡逻点单次查找失败不代表巡逻行为失效，保持巡逻意图并在下一决策周期重试。
	return ELxAIBehaviorExecutionResult::Waiting;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot)
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

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot)
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
	const float AttackSkillRange = ConvertModuleMetersToWorldDistance(Config.AttackSkillRange);
	if (Distance > AttackSkillRange)
	{
		if (CharacterBehaviorControlComponent->IsNavigationMoving())
		{
			return ELxAIBehaviorExecutionResult::InProgress;
		}
		const float AcceptanceRadius = FMath::Min(
			ConvertModuleMetersToWorldDistance(Config.AttackAcceptanceRadius), AttackSkillRange);
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

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot)
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
		if (CharacterBehaviorControlComponent->IsNavigationMoving())
		{
			return ELxAIBehaviorExecutionResult::InProgress;
		}
		return CharacterBehaviorControlComponent->RequestMoveToLocation(InBattleSnapshot.AssistCenter, 150.0f) ?
			ELxAIBehaviorExecutionResult::Started : ELxAIBehaviorExecutionResult::Failed;
	}
	CharacterBehaviorControlComponent->StopActiveMovement();
	return ELxAIBehaviorExecutionResult::InProgress;
}

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot)
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
	const float HealSkillRange = ConvertModuleMetersToWorldDistance(Config.HealSkillRange);
	if (Distance > HealSkillRange)
	{
		if (CharacterBehaviorControlComponent->IsNavigationMoving())
		{
			return ELxAIBehaviorExecutionResult::InProgress;
		}
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

ELxAIBehaviorExecutionResult ULxAIBehaviorModule::ExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot)
{
	AActor* NearestEnemy = InBattleSnapshot.NearestEnemy;
	if (!bRetreatInProgress && !IsValid(NearestEnemy))
	{
		return ELxAIBehaviorExecutionResult::Failed;
	}

	const FVector CurrentLocation = OwnerAICharacter->GetActorLocation();
	const FVector CurrentNavigationLocation = GetModuleNavigationAgentLocation(OwnerAICharacter);
	const bool bStartingRetreat = !bRetreatInProgress;

	if (CharacterBehaviorControlComponent->IsNavigationMoving())
	{
		return ELxAIBehaviorExecutionResult::InProgress;
	}

	const FVector EnemyLocation = IsValid(NearestEnemy) ? NearestEnemy->GetActorLocation() :
		CurrentLocation - LastRetreatDirection * ConvertModuleMetersToWorldDistance(ModuleMinimumRetreatMoveDistanceMeters);
	FVector RetreatDirection = IsValid(NearestEnemy) ?
		(CurrentLocation - EnemyLocation).GetSafeNormal2D() : LastRetreatDirection;
	if (RetreatDirection.IsNearlyZero())
	{
		RetreatDirection = -OwnerAICharacter->GetActorForwardVector();
	}
	LastRetreatDirection = RetreatDirection.GetSafeNormal2D();
	const float RequiredRetreatDistance = ConvertModuleMetersToWorldDistance(
		OwnerAICharacter->GetAIControlConfig().RetreatDistance);
	// 正常情况下直接请求全部剩余逃跑距离，避免固定3米分段之间等待下一次决策造成停顿。
	const float RemainingRetreatDistance = FMath::Max(
		ConvertModuleMetersToWorldDistance(ModuleMinimumRetreatMoveDistanceMeters),
		RequiredRetreatDistance - AccumulatedRetreatDistance);

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		const FNavAgentProperties& AgentProperties = OwnerAICharacter->GetNavAgentPropertiesRef();
		ANavigationData* NavigationData = NavigationSystem->GetNavDataForProps(
			AgentProperties, CurrentNavigationLocation);
		const FVector RetreatProjectionExtent = GetModuleNavigationProjectionExtent(
			OwnerAICharacter, NavigationData);
		if (!NavigationData)
		{
			return ELxAIBehaviorExecutionResult::Failed;
		}
		for (const float DirectionAngle : ModuleRetreatDirectionAngles)
		{
			const FVector CandidateDirection = RetreatDirection.RotateAngleAxis(DirectionAngle, FVector::UpVector);
			for (const float DistanceScale : ModuleRetreatDistanceScales)
			{
				const FVector DesiredLocation = CurrentNavigationLocation +
					CandidateDirection * RemainingRetreatDistance * DistanceScale;
				FNavLocation ReachableLocation;
				if (!NavigationSystem->ProjectPointToNavigation(
					DesiredLocation, ReachableLocation, RetreatProjectionExtent, NavigationData))
				{
					continue;
				}
				if (FVector::DistSquared2D(ReachableLocation.Location, EnemyLocation) <=
					FVector::DistSquared2D(CurrentLocation, EnemyLocation))
				{
					continue;
				}

				const UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(
					this, CurrentNavigationLocation, ReachableLocation.Location, OwnerAICharacter);
				if (!IsCompleteModuleRetreatPath(NavigationPath))
				{
					continue;
				}

				if (AAIController* AIController = GetOwnerAIController())
				{
					AIController->ClearFocus(EAIFocusPriority::Gameplay);
				}
				if (CharacterBehaviorControlComponent->RequestMoveToLocation(ReachableLocation.Location, 35.0f))
				{
					if (bStartingRetreat)
					{
						bPatrolOriginNeedsRefresh = true;
						bRetreatInProgress = true;
						RetreatStartLocation = CurrentLocation;
						LastRetreatSampleLocation = CurrentLocation;
						AccumulatedRetreatDistance = 0.0f;
						LastRetreatEnemy = NearestEnemy;
						LastRetreatEnemyDistance = FVector::Dist2D(CurrentLocation, NearestEnemy->GetActorLocation());
						bHadRetreatEnemy = true;
						bRetreatEnemyWasClosing = false;
					}
					return ELxAIBehaviorExecutionResult::Started;
				}
			}
		}
	}
	if (bRetreatInProgress)
	{
		ResetRetreatState();
	}
	return ELxAIBehaviorExecutionResult::Failed;
}

void ULxAIBehaviorModule::ResetRetreatState()
{
	bRetreatInProgress = false;
	RetreatStartLocation = FVector::ZeroVector;
	LastRetreatSampleLocation = FVector::ZeroVector;
	AccumulatedRetreatDistance = 0.0f;
	LastRetreatDirection = FVector::ZeroVector;
	LastRetreatEnemy.Reset();
	LastRetreatEnemyDistance = 0.0f;
	bHadRetreatEnemy = false;
	bRetreatEnemyWasClosing = false;
	bRetreatCompletionBlocked = false;
	CompletedRetreatEnemy.Reset();
	CompletedRetreatEnemyDistance = 0.0f;
}

AAIController* ULxAIBehaviorModule::GetOwnerAIController() const
{
	return OwnerAICharacter ? Cast<AAIController>(OwnerAICharacter->GetController()) : nullptr;
}
