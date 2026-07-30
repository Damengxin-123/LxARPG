#include "LxAIController.h"

#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIGroupSubsystem.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	/** 构建单个目标参与群体分析时使用的临时数据。 */
	struct FLxAnalyzedCharacter
	{
		TObjectPtr<ALxBaseCharacter> Character = nullptr;
		float CombatPower = 0.0f;
		float HealthRatio = 1.0f;
	};
}

ALxAIController::ALxAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2500.0f;
	SightConfig->LoseSightRadius = 3000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ALxAIController::HandleTargetPerceptionUpdated);
}

void ALxAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter)
	{
		return;
	}

	PatrolOrigin = AICharacter->GetActorLocation();
	RuntimeGroupId = ResolveRuntimeGroupId(AICharacter);
	if (ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
	{
		GroupSubsystem->RegisterMember(RuntimeGroupId, AICharacter);
	}

	ApplyPerceptionConfiguration();
	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	if (Config.bEnableAutomaticControl)
	{
		const float DecisionInterval = FMath::Max(0.05f, Config.DecisionInterval);
		GetWorldTimerManager().SetTimer(AutomaticDecisionTimer, this, &ALxAIController::RunAutomaticDecision,
			DecisionInterval, true, FMath::FRandRange(0.01f, DecisionInterval));
	}
}

void ALxAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(AutomaticDecisionTimer);
	if (ALxAICharacter* AICharacter = GetAICharacter())
	{
		if (ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
		{
			GroupSubsystem->SetMemberAction(RuntimeGroupId, AICharacter, ELxAIActionType::None);
			GroupSubsystem->UnregisterMember(RuntimeGroupId, AICharacter);
		}
	}
	RuntimeGroupId = NAME_None;
	DynamicHostileTargets.Reset();
	Super::OnUnPossess();
}

void ALxAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutomaticDecisionTimer);
	Super::EndPlay(EndPlayReason);
}

void ALxAIController::HandleTargetPerceptionUpdated(AActor* InActor, const FAIStimulus InStimulus)
{
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !IsValid(InActor) || !InStimulus.WasSuccessfullySensed())
	{
		return;
	}

	const bool bHostileBehavior = InStimulus.Type == UAISense::GetSenseID<UAISense_Damage>();
	if (bHostileBehavior)
	{
		DynamicHostileTargets.Add(InActor);
	}

	if (ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
	{
		GroupSubsystem->ReportSensedTarget(RuntimeGroupId, AICharacter, InActor, bHostileBehavior);
	}
}

void ALxAIController::RunAutomaticDecision()
{
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !AICharacter->HasAuthority() || !AICharacter->GetAIControlConfig().bEnableAutomaticControl)
	{
		return;
	}

	CurrentBattleSnapshot = BuildBattleSnapshot();
	const ELxAITacticalStrategy NewStrategy = EvaluateStrategy(CurrentBattleSnapshot);
	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	const double CurrentTime = GetWorld()->GetTimeSeconds();

	const FLxAIActionRule* CurrentRule = Config.ActionRules.FindByPredicate([this](const FLxAIActionRule& Rule)
	{
		return Rule.ActionType == CurrentAction;
	});
	if (CurrentAction != ELxAIActionType::None && NewStrategy == CurrentStrategy && CurrentRule &&
		CurrentTime - CurrentActionStartTime < CurrentRule->MinExecutionTime)
	{
		ExecuteCurrentAction();
		return;
	}

	float BestScore = 0.0f;
	ELxAIActionType BestAction = SelectBestAction(CurrentBattleSnapshot, NewStrategy, BestScore);
	if (BestAction == ELxAIActionType::None)
	{
		BestAction = NewStrategy == ELxAITacticalStrategy::Idle ? ELxAIActionType::Alert :
			(NewStrategy == ELxAITacticalStrategy::Escape ? ELxAIActionType::Retreat : ELxAIActionType::Attack);
	}

	if (BestAction != CurrentAction && NewStrategy == CurrentStrategy && BestScore < CurrentActionScore + Config.ActionSwitchScoreMargin)
	{
		BestAction = CurrentAction;
		BestScore = CurrentActionScore;
	}

	ChangeAction(NewStrategy, BestAction, BestScore);
	ExecuteCurrentAction();
}

void ALxAIController::ApplyPerceptionConfiguration()
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !SightConfig || !AIPerceptionComponent)
	{
		return;
	}

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	SightConfig->SightRadius = FMath::Max(0.0f, Config.SightRadius);
	SightConfig->LoseSightRadius = FMath::Max(SightConfig->SightRadius, Config.LoseSightRadius);
	SightConfig->SetMaxAge(FMath::Max(0.1f, Config.SharedPerceptionMaxAge));
	DamageConfig->SetMaxAge(FMath::Max(0.1f, Config.SharedPerceptionMaxAge));
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->RequestStimuliListenerUpdate();
}

FLxAIBattleSnapshot ALxAIController::BuildBattleSnapshot() const
{
	FLxAIBattleSnapshot Snapshot;
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter)
	{
		return Snapshot;
	}

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	TArray<AActor*> SharedTargets;
	TArray<ALxAICharacter*> GroupMembers;
	if (const ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
	{
		GroupSubsystem->GetSharedTargets(RuntimeGroupId, Config.SharedPerceptionMaxAge, SharedTargets);
		GroupSubsystem->GetGroupMembers(RuntimeGroupId, GroupMembers);
	}

	TArray<FLxAnalyzedCharacter> EnemyCharacters;
	TArray<FLxAnalyzedCharacter> AssistCharacters;
	TSet<const ALxBaseCharacter*> AddedCharacters;

	auto AddAnalyzedCharacter = [&AddedCharacters](ALxBaseCharacter* InAnalyzedCharacter, TArray<FLxAnalyzedCharacter>& TargetList)
	{
		if (!IsValid(InAnalyzedCharacter) || AddedCharacters.Contains(InAnalyzedCharacter))
		{
			return;
		}
		AddedCharacters.Add(InAnalyzedCharacter);
		FLxAnalyzedCharacter Analysis;
		Analysis.Character = InAnalyzedCharacter;
		Analysis.HealthRatio = ALxAIController::GetHealthRatioForCharacter(InAnalyzedCharacter);
		Analysis.CombatPower = ALxAIController::GetCombatPowerForCharacter(InAnalyzedCharacter);
		TargetList.Add(Analysis);
	};

	AddAnalyzedCharacter(const_cast<ALxAICharacter*>(AICharacter), AssistCharacters);
	for (ALxAICharacter* Member : GroupMembers)
	{
		AddAnalyzedCharacter(Member, AssistCharacters);
	}

	for (AActor* SharedTarget : SharedTargets)
	{
		ALxBaseCharacter* TargetCharacter = Cast<ALxBaseCharacter>(SharedTarget);
		if (!TargetCharacter)
		{
			continue;
		}

		switch (ResolveTargetRelation(TargetCharacter))
		{
		case ELxAITargetRelation::Hostile:
			AddAnalyzedCharacter(TargetCharacter, EnemyCharacters);
			break;
		case ELxAITargetRelation::Assist:
			AddAnalyzedCharacter(TargetCharacter, AssistCharacters);
			break;
		default:
			break;
		}
	}

	auto AccumulateSide = [](const TArray<FLxAnalyzedCharacter>& Characters, int32& OutCount, float& OutPower, FVector& OutCenter)
	{
		OutCount = Characters.Num();
		OutPower = 0.0f;
		OutCenter = FVector::ZeroVector;
		for (const FLxAnalyzedCharacter& Character : Characters)
		{
			OutPower += Character.CombatPower;
			OutCenter += Character.Character->GetActorLocation();
		}
		if (OutCount > 0)
		{
			OutCenter /= static_cast<float>(OutCount);
		}
	};

	AccumulateSide(EnemyCharacters, Snapshot.EnemyCount, Snapshot.EnemyPower, Snapshot.EnemyCenter);
	AccumulateSide(AssistCharacters, Snapshot.AssistCount, Snapshot.AssistPower, Snapshot.AssistCenter);
	const float TotalPower = Snapshot.EnemyPower + Snapshot.AssistPower;
	Snapshot.AdvantageScore = TotalPower > UE_SMALL_NUMBER ?
		(Snapshot.AssistPower - Snapshot.EnemyPower) / TotalPower : 0.0f;
	Snapshot.bHasThreat = Snapshot.EnemyCount > 0;

	float HighestThreatScore = -1.0f;
	for (const FLxAnalyzedCharacter& Enemy : EnemyCharacters)
	{
		const float DistanceScale = FMath::Max(1.0f, FVector::Dist(AICharacter->GetActorLocation(), Enemy.Character->GetActorLocation()) / 100.0f);
		const float ThreatScore = Enemy.CombatPower / DistanceScale;
		if (ThreatScore > HighestThreatScore)
		{
			HighestThreatScore = ThreatScore;
			Snapshot.HighestThreatEnemy = Enemy.Character;
		}
	}

	for (const FLxAnalyzedCharacter& Assist : AssistCharacters)
	{
		if (Assist.Character != AICharacter && Assist.HealthRatio < Snapshot.LowestAllyHealthRatio &&
			Assist.HealthRatio <= Config.InjuredAllyThreshold)
		{
			Snapshot.LowestAllyHealthRatio = Assist.HealthRatio;
			Snapshot.LowestStateAlly = Assist.Character;
		}
	}

	auto BuildIntentSummary = [&Config](const TArray<FLxAnalyzedCharacter>& Characters, const FVector& OpposingCenter,
		const bool bHasOpposingSide)
	{
		FLxAIGroupIntentSummary Summary;
		if (Characters.IsEmpty() || !bHasOpposingSide)
		{
			return Summary;
		}

		float AdvancePower = 0.0f;
		float DefendPower = 0.0f;
		float RetreatPower = 0.0f;
		for (const FLxAnalyzedCharacter& Character : Characters)
		{
			const FVector Velocity = Character.Character->GetVelocity();
			const float Speed = Velocity.Size2D();
			const FVector DirectionToOpposition = (OpposingCenter - Character.Character->GetActorLocation()).GetSafeNormal2D();
			const float RadialSpeed = FVector::DotProduct(Velocity, DirectionToOpposition);
			if (Speed <= Config.IntentStationarySpeed || FMath::Abs(RadialSpeed) < Config.IntentRadialSpeed)
			{
				DefendPower += Character.CombatPower;
			}
			else if (RadialSpeed > 0.0f)
			{
				AdvancePower += Character.CombatPower;
			}
			else
			{
				RetreatPower += Character.CombatPower;
			}
		}

		const float IntentPower = AdvancePower + DefendPower + RetreatPower;
		if (IntentPower <= UE_SMALL_NUMBER)
		{
			return Summary;
		}

		Summary.AdvanceRatio = AdvancePower / IntentPower;
		Summary.DefendRatio = DefendPower / IntentPower;
		Summary.RetreatRatio = RetreatPower / IntentPower;
		float HighestRatio = Summary.AdvanceRatio;
		float SecondRatio = FMath::Max(Summary.DefendRatio, Summary.RetreatRatio);
		Summary.DominantIntent = ELxAIMovementIntent::Advance;
		if (Summary.DefendRatio > HighestRatio)
		{
			SecondRatio = FMath::Max(HighestRatio, Summary.RetreatRatio);
			HighestRatio = Summary.DefendRatio;
			Summary.DominantIntent = ELxAIMovementIntent::Defend;
		}
		if (Summary.RetreatRatio > HighestRatio)
		{
			SecondRatio = FMath::Max(HighestRatio, Summary.DefendRatio);
			HighestRatio = Summary.RetreatRatio;
			Summary.DominantIntent = ELxAIMovementIntent::Retreat;
		}
		Summary.Dominance = FMath::Max(0.0f, HighestRatio - SecondRatio);
		Summary.Confidence = FMath::Clamp(static_cast<float>(Characters.Num()) / 3.0f, 0.25f, 1.0f);
		return Summary;
	};

	Snapshot.EnemyIntent = BuildIntentSummary(EnemyCharacters, Snapshot.AssistCenter, Snapshot.AssistCount > 0);
	Snapshot.AssistIntent = BuildIntentSummary(AssistCharacters, Snapshot.EnemyCenter, Snapshot.EnemyCount > 0);
	return Snapshot;
}

ELxAITargetRelation ALxAIController::ResolveTargetRelation(const ALxBaseCharacter* InTargetCharacter) const
{
	if (!InTargetCharacter)
	{
		return ELxAITargetRelation::Ignore;
	}
	if (DynamicHostileTargets.Contains(InTargetCharacter))
	{
		return ELxAITargetRelation::Hostile;
	}
	if (const ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
	{
		if (GroupSubsystem->IsTargetMarkedHostile(RuntimeGroupId, InTargetCharacter))
		{
			return ELxAITargetRelation::Hostile;
		}
	}
	const ALxAICharacter* AICharacter = GetAICharacter();
	return AICharacter ? AICharacter->ResolveBaseTargetRelation(InTargetCharacter) : ELxAITargetRelation::Ignore;
}

ELxAITacticalStrategy ALxAIController::EvaluateStrategy(const FLxAIBattleSnapshot& InSnapshot) const
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !InSnapshot.bHasThreat)
	{
		return ELxAITacticalStrategy::Idle;
	}

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	if (InSnapshot.AdvantageScore <= Config.EscapeAdvantageThreshold ||
		AICharacter->GetCurrentHealthRatio() <= Config.EscapeHealthThreshold)
	{
		return ELxAITacticalStrategy::Escape;
	}
	return ELxAITacticalStrategy::Engage;
}

ELxAIActionType ALxAIController::SelectBestAction(const FLxAIBattleSnapshot& InSnapshot,
	const ELxAITacticalStrategy InStrategy, float& OutBestScore) const
{
	OutBestScore = -TNumericLimits<float>::Max();
	ELxAIActionType BestAction = ELxAIActionType::None;
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter)
	{
		return BestAction;
	}

	for (const FLxAIActionRule& Rule : AICharacter->GetAIControlConfig().ActionRules)
	{
		if (!IsActionRuleAvailable(Rule, InSnapshot, InStrategy))
		{
			continue;
		}
		const float Score = CalculateActionScore(Rule, InSnapshot);
		if (Score > OutBestScore)
		{
			OutBestScore = Score;
			BestAction = Rule.ActionType;
		}
	}
	return BestAction;
}

bool ALxAIController::IsActionRuleAvailable(const FLxAIActionRule& InRule, const FLxAIBattleSnapshot& InSnapshot,
	const ELxAITacticalStrategy InStrategy) const
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !InRule.bEnabled || InRule.ActionType == ELxAIActionType::None || !InRule.AllowsStrategy(InStrategy))
	{
		return false;
	}

	const float SelfHealthRatio = AICharacter->GetCurrentHealthRatio();
	if (InSnapshot.AdvantageScore < InRule.MinAdvantage || InSnapshot.AdvantageScore > InRule.MaxAdvantage ||
		SelfHealthRatio < InRule.MinSelfHealthRatio || SelfHealthRatio > InRule.MaxSelfHealthRatio ||
		(InRule.bRequiresInjuredAlly && !IsValid(InSnapshot.LowestStateAlly)))
	{
		return false;
	}

	if (InRule.ActionType == ELxAIActionType::Heal && !AICharacter->GetAIControlConfig().HealSkillItemId.IsValid())
	{
		return false;
	}

	if (InRule.MaxGroupExecutors > 0 && InRule.ActionType != CurrentAction)
	{
		if (const ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
		{
			if (GroupSubsystem->GetActionExecutorCount(RuntimeGroupId, InRule.ActionType) >= InRule.MaxGroupExecutors)
			{
				return false;
			}
		}
	}

	if (InRule.ActionType != CurrentAction)
	{
		if (const double* LastEndTime = ActionEndTimes.Find(InRule.ActionType))
		{
			if (GetWorld()->GetTimeSeconds() - *LastEndTime < InRule.Cooldown)
			{
				return false;
			}
		}
	}
	return true;
}

float ALxAIController::CalculateActionScore(const FLxAIActionRule& InRule, const FLxAIBattleSnapshot& InSnapshot) const
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	const float SelfInjuryRatio = AICharacter ? 1.0f - AICharacter->GetCurrentHealthRatio() : 0.0f;
	const float InjuredAllyRatio = IsValid(InSnapshot.LowestStateAlly) ? 1.0f - InSnapshot.LowestAllyHealthRatio : 0.0f;
	return InRule.BaseScore +
		InSnapshot.AdvantageScore * InRule.AdvantageWeight +
		InSnapshot.EnemyIntent.AdvanceRatio * InRule.EnemyAdvanceRatioWeight +
		InSnapshot.AssistIntent.AdvanceRatio * InRule.AssistAdvanceRatioWeight +
		InjuredAllyRatio * InRule.InjuredAllyWeight +
		SelfInjuryRatio * InRule.SelfInjuryWeight;
}

void ALxAIController::ChangeAction(const ELxAITacticalStrategy InStrategy, const ELxAIActionType InActionType,
	const float InActionScore)
{
	const bool bChanged = CurrentStrategy != InStrategy || CurrentAction != InActionType;
	if (!bChanged)
	{
		CurrentActionScore = InActionScore;
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentAction != ELxAIActionType::None)
	{
		ActionEndTimes.FindOrAdd(CurrentAction) = CurrentTime;
	}
	CurrentStrategy = InStrategy;
	CurrentAction = InActionType;
	CurrentActionScore = InActionScore;
	CurrentActionStartTime = CurrentTime;

	if (ALxAICharacter* AICharacter = GetAICharacter())
	{
		if (ULxAIGroupSubsystem* GroupSubsystem = GetWorld()->GetSubsystem<ULxAIGroupSubsystem>())
		{
			GroupSubsystem->SetMemberAction(RuntimeGroupId, AICharacter, CurrentAction);
		}
	}
	OnAIActionChanged.Broadcast(CurrentStrategy, CurrentAction);
}

void ALxAIController::ExecuteCurrentAction()
{
	switch (CurrentAction)
	{
	case ELxAIActionType::Patrol:
		ExecutePatrolAction();
		break;
	case ELxAIActionType::Alert:
		ExecuteAlertAction();
		break;
	case ELxAIActionType::Attack:
		ExecuteAttackAction();
		break;
	case ELxAIActionType::Defend:
		ExecuteDefendAction();
		break;
	case ELxAIActionType::Heal:
		ExecuteHealAction();
		break;
	case ELxAIActionType::Retreat:
		ExecuteRetreatAction();
		break;
	default:
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		break;
	}
}

void ALxAIController::ExecutePatrolAction()
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	FNavLocation PatrolLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		if (NavigationSystem->GetRandomReachablePointInRadius(PatrolOrigin, AICharacter->GetAIControlConfig().PatrolRadius, PatrolLocation))
		{
			MoveToLocation(PatrolLocation.Location, 50.0f, true, true, false, true, nullptr, true);
		}
	}
}

void ALxAIController::ExecuteAlertAction()
{
	StopMovement();
	if (IsValid(CurrentBattleSnapshot.HighestThreatEnemy))
	{
		SetFocus(CurrentBattleSnapshot.HighestThreatEnemy);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void ALxAIController::ExecuteAttackAction()
{
	ALxAICharacter* AICharacter = GetAICharacter();
	AActor* TargetActor = CurrentBattleSnapshot.HighestThreatEnemy;
	if (!AICharacter || !IsValid(TargetActor))
	{
		ExecuteAlertAction();
		return;
	}

	SetFocus(TargetActor);
	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	const float Distance = FVector::Dist(AICharacter->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > Config.AttackAcceptanceRadius)
	{
		MoveToActor(TargetActor, Config.AttackAcceptanceRadius, true, true, true, nullptr, true);
	}
	else
	{
		StopMovement();
	}

	if (Distance <= Config.AttackSkillRange && Config.AttackSkillItemId.IsValid())
	{
		ULxSkillBackpackComponent* SkillBackpack = AICharacter->GetSkillBackpackComponent();
		ULxSkillCastComponent* SkillCast = AICharacter->GetSkillCastComponent();
		if (SkillBackpack && SkillCast && SkillCast->IsSkillCastIdle())
		{
			if (ULxSkillItem* SkillItem = SkillBackpack->FindSkillItemByTagID(Config.AttackSkillItemId))
			{
				const FVector AimDirection = (TargetActor->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal();
				const FLxSkillCastContext CastContext = SkillCast->MakeSkillCastContext(AICharacter, TargetActor,
					TargetActor->GetActorLocation(), true, AimDirection, true);
				SkillCast->ReleaseSkillItemDirectly(SkillItem, CastContext);
			}
		}
	}
}

void ALxAIController::ExecuteDefendAction()
{
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter)
	{
		return;
	}

	if (IsValid(CurrentBattleSnapshot.HighestThreatEnemy))
	{
		SetFocus(CurrentBattleSnapshot.HighestThreatEnemy);
	}
	if (FVector::DistSquared2D(AICharacter->GetActorLocation(), CurrentBattleSnapshot.AssistCenter) > FMath::Square(250.0f))
	{
		MoveToLocation(CurrentBattleSnapshot.AssistCenter, 150.0f, true, true, true, true, nullptr, true);
	}
	else
	{
		StopMovement();
	}
}

void ALxAIController::ExecuteHealAction()
{
	ALxAICharacter* AICharacter = GetAICharacter();
	AActor* HealTarget = CurrentBattleSnapshot.LowestStateAlly;
	if (!AICharacter || !IsValid(HealTarget))
	{
		ExecuteDefendAction();
		return;
	}

	SetFocus(HealTarget);
	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	const float Distance = FVector::Dist(AICharacter->GetActorLocation(), HealTarget->GetActorLocation());
	if (Distance > Config.HealSkillRange)
	{
		MoveToActor(HealTarget, Config.HealSkillRange * 0.8f, true, true, true, nullptr, true);
		return;
	}

	StopMovement();
	ULxSkillBackpackComponent* SkillBackpack = AICharacter->GetSkillBackpackComponent();
	ULxSkillCastComponent* SkillCast = AICharacter->GetSkillCastComponent();
	if (SkillBackpack && SkillCast && SkillCast->IsSkillCastIdle())
	{
		if (ULxSkillItem* SkillItem = SkillBackpack->FindSkillItemByTagID(Config.HealSkillItemId))
		{
			const FVector AimDirection = (HealTarget->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal();
			const FLxSkillCastContext CastContext = SkillCast->MakeSkillCastContext(AICharacter, HealTarget,
				HealTarget->GetActorLocation(), true, AimDirection, true);
			SkillCast->ReleaseSkillItemDirectly(SkillItem, CastContext);
		}
	}
}

void ALxAIController::ExecuteRetreatAction()
{
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	FVector RetreatDirection = (AICharacter->GetActorLocation() - CurrentBattleSnapshot.EnemyCenter).GetSafeNormal2D();
	if (RetreatDirection.IsNearlyZero())
	{
		RetreatDirection = -AICharacter->GetActorForwardVector();
	}
	const FVector DesiredLocation = AICharacter->GetActorLocation() +
		RetreatDirection * AICharacter->GetAIControlConfig().RetreatDistance;

	FNavLocation ReachableLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ReachableLocation))
		{
			ClearFocus(EAIFocusPriority::Gameplay);
			MoveToLocation(ReachableLocation.Location, 50.0f, true, true, false, true, nullptr, true);
		}
	}
}

float ALxAIController::GetHealthRatioForCharacter(const ALxBaseCharacter* InCharacter)
{
	if (!InCharacter)
	{
		return 0.0f;
	}
	if (const ALxAICharacter* AICharacter = Cast<ALxAICharacter>(InCharacter))
	{
		return AICharacter->GetCurrentHealthRatio();
	}

	const ULxCharacterAttributeComponent* AttributeComponent = InCharacter->GetCharacterAttributeComponent();
	const ULxCharacterBaseAttributeSet* AttributeSet = AttributeComponent ? AttributeComponent->GetRuntimeAttributeSet() : nullptr;
	FLxResourceAttributeData HealthAttribute;
	if (!AttributeSet || !AttributeSet->GetResourceAttribute(LxTag_Attribute_Resource_Health, HealthAttribute) ||
		HealthAttribute.ValueLimit <= UE_SMALL_NUMBER)
	{
		return 1.0f;
	}
	return FMath::Clamp(HealthAttribute.Value / HealthAttribute.ValueLimit, 0.0f, 1.0f);
}

float ALxAIController::GetCombatPowerForCharacter(const ALxBaseCharacter* InCharacter)
{
	if (!InCharacter)
	{
		return 0.0f;
	}
	if (const ALxAICharacter* AICharacter = Cast<ALxAICharacter>(InCharacter))
	{
		return AICharacter->CalculateEffectiveCombatPower();
	}
	const ULxCharacterAttributeComponent* AttributeComponent = InCharacter->GetCharacterAttributeComponent();
	const float TotalStrength = AttributeComponent ? static_cast<float>(AttributeComponent->CalculateTotalStrength()) : 0.0f;
	return FMath::Max(1.0f, TotalStrength) * GetHealthRatioForCharacter(InCharacter);
}

ALxAICharacter* ALxAIController::GetAICharacter() const
{
	return Cast<ALxAICharacter>(GetPawn());
}

FName ALxAIController::ResolveRuntimeGroupId(const ALxAICharacter* InCharacter) const
{
	if (!InCharacter)
	{
		return NAME_None;
	}
	if (!InCharacter->GetAIGroupId().IsNone())
	{
		return InCharacter->GetAIGroupId();
	}
	return FName(*FString::Printf(TEXT("AI_Solo_%u"), InCharacter->GetUniqueID()));
}
