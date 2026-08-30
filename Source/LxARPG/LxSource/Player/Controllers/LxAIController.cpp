#include "LxAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorModule.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	/** 将AI业务配置使用的米换算为虚幻世界单位厘米。 */
	constexpr float MetersToCentimeters = 100.0f;

	/** 单个目标参与当前AI数值汇总时使用的临时数据。 */
	struct FLxAnalyzedTarget
	{
		/** 被当前AI直接感知到的角色。 */
		TObjectPtr<ALxBaseCharacter> Character = nullptr;

		/** 角色未乘当前状态前的基础强度。 */
		float BaseStrength = 0.0f;

		/** 角色当前生命值对应的归一化状态。 */
		float StateRatio = 1.0f;

		/** 基础强度乘状态比例后的有效强度。 */
		float EffectiveStrength = 0.0f;
	};
}

ALxAIController::ALxAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 25.0f * MetersToCentimeters;
	SightConfig->LoseSightRadius = 30.0f * MetersToCentimeters;
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

	TargetMemory.Reset();
	DynamicHostileTargets.Reset();
	CurrentSituation = ELxAISituationLevel::NoThreat;
	CurrentAction = ELxAIActionType::None;
	CurrentActionStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	CurrentBattleSnapshot = FLxAIBattleSnapshot();
	ApplyPerceptionConfiguration();

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	if (Config.bEnableAutomaticControl)
	{
		// 接管角色时立即建立首个行为，避免在随机首次定时延迟内对外显示“无”。
		RunAutomaticDecision();
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
		if (ULxAIBehaviorModule* BehaviorComponent = AICharacter->GetAIBehaviorComponent())
		{
			BehaviorComponent->StopBehavior();
		}
	}
	TargetMemory.Reset();
	DynamicHostileTargets.Reset();
	CurrentSituation = ELxAISituationLevel::NoThreat;
	CurrentAction = ELxAIActionType::None;
	CurrentBattleSnapshot = FLxAIBattleSnapshot();
	Super::OnUnPossess();
}

void ALxAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutomaticDecisionTimer);
	Super::EndPlay(EndPlayReason);
}

void ALxAIController::ReportPerceivedTarget(AActor* InTargetActor, const ELxAIPerceptionSource InPerceptionSource,
	const bool bInMarkAsHostile)
{
	ALxBaseCharacter* TargetCharacter = Cast<ALxBaseCharacter>(InTargetActor);
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!IsValid(TargetCharacter) || !AICharacter || TargetCharacter == AICharacter)
	{
		return;
	}

	FLxAITargetMemoryRecord& Record = TargetMemory.FindOrAdd(InTargetActor);
	Record.TargetCharacter = TargetCharacter;
	Record.PerceptionSource = InPerceptionSource;
	Record.LastSensedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Record.bHostileByDamage |= bInMarkAsHostile;
	if (bInMarkAsHostile)
	{
		DynamicHostileTargets.Add(InTargetActor);
	}
}

void ALxAIController::HandleTargetPerceptionUpdated(AActor* InActor, const FAIStimulus InStimulus)
{
	if (!IsValid(InActor) || !InStimulus.WasSuccessfullySensed())
	{
		return;
	}

	const bool bDamageSource = InStimulus.Type == UAISense::GetSenseID<UAISense_Damage>();
	const bool bSightSource = InStimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
	const ELxAIPerceptionSource PerceptionSource = bDamageSource ? ELxAIPerceptionSource::Damage :
		(bSightSource ? ELxAIPerceptionSource::Sight : ELxAIPerceptionSource::Unknown);
	ReportPerceivedTarget(InActor, PerceptionSource, bDamageSource);
}

void ALxAIController::RunAutomaticDecision()
{
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !AICharacter->HasAuthority() || !AICharacter->GetAIControlConfig().bEnableAutomaticControl)
	{
		return;
	}

	RefreshActivePerceptionMemory();
	CurrentBattleSnapshot = BuildBattleSnapshot();
	const ELxAISituationLevel NewSituation = EvaluateSituation(CurrentBattleSnapshot);
	SelectAndExecuteAction(NewSituation);
}

void ALxAIController::ApplyPerceptionConfiguration()
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !SightConfig || !DamageConfig || !AIPerceptionComponent)
	{
		return;
	}

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	SightConfig->SightRadius = FMath::Max(0.0f, Config.SightRadius) * MetersToCentimeters;
	SightConfig->LoseSightRadius = FMath::Max(Config.SightRadius, Config.LoseSightRadius) * MetersToCentimeters;
	SightConfig->SetMaxAge(FMath::Max(0.1f, Config.TargetMemoryMaxAge));
	DamageConfig->SetMaxAge(FMath::Max(0.1f, Config.TargetMemoryMaxAge));
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->RequestStimuliListenerUpdate();
}

void ALxAIController::RefreshActivePerceptionMemory()
{
	if (!AIPerceptionComponent)
	{
		return;
	}

	TArray<AActor*> CurrentlyPerceivedActors;
	AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), CurrentlyPerceivedActors);
	for (AActor* PerceivedActor : CurrentlyPerceivedActors)
	{
		ReportPerceivedTarget(PerceivedActor, ELxAIPerceptionSource::Sight);
	}
}

void ALxAIController::PruneTargetMemory()
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const double MaxAge = AICharacter ? FMath::Max(0.1f, AICharacter->GetAIControlConfig().TargetMemoryMaxAge) : 0.1;

	for (auto Iterator = TargetMemory.CreateIterator(); Iterator; ++Iterator)
	{
		const FLxAITargetMemoryRecord& Record = Iterator.Value();
		if (!Record.TargetCharacter.IsValid() || CurrentTime - Record.LastSensedTime > MaxAge)
		{
			DynamicHostileTargets.Remove(Iterator.Key());
			Iterator.RemoveCurrent();
		}
	}
}

FLxAIBattleSnapshot ALxAIController::BuildBattleSnapshot()
{
	FLxAIBattleSnapshot Snapshot;
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter)
	{
		return Snapshot;
	}

	PruneTargetMemory();
	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	TArray<FLxAnalyzedTarget> EnemyTargets;
	TArray<FLxAnalyzedTarget> AssistTargets;
	TSet<const ALxBaseCharacter*> AddedCharacters;

	auto AddAnalyzedTarget = [&AddedCharacters](ALxBaseCharacter* InTargetCharacter, TArray<FLxAnalyzedTarget>& TargetList)
	{
		if (!IsValid(InTargetCharacter) || AddedCharacters.Contains(InTargetCharacter))
		{
			return;
		}
		AddedCharacters.Add(InTargetCharacter);
		FLxAnalyzedTarget Analysis;
		Analysis.Character = InTargetCharacter;
		Analysis.StateRatio = ALxAIController::GetStateRatioForCharacter(InTargetCharacter);
		Analysis.BaseStrength = ALxAIController::GetBaseStrengthForCharacter(InTargetCharacter);
		Analysis.EffectiveStrength = Analysis.BaseStrength * Analysis.StateRatio;
		TargetList.Add(Analysis);
	};

	AddAnalyzedTarget(AICharacter, AssistTargets);
	for (const TPair<TWeakObjectPtr<AActor>, FLxAITargetMemoryRecord>& Pair : TargetMemory)
	{
		ALxBaseCharacter* TargetCharacter = Pair.Value.TargetCharacter.Get();
		switch (ResolveTargetRelation(TargetCharacter))
		{
		case ELxAITargetRelation::Hostile:
			AddAnalyzedTarget(TargetCharacter, EnemyTargets);
			break;
		case ELxAITargetRelation::Assist:
			AddAnalyzedTarget(TargetCharacter, AssistTargets);
			break;
		default:
			break;
		}
	}

	auto AccumulateSide = [](const TArray<FLxAnalyzedTarget>& Targets, int32& OutCount, float& OutBaseStrength,
		float& OutEffectiveStrength, float& OutAverageState, FVector& OutCenter)
	{
		OutCount = Targets.Num();
		OutBaseStrength = 0.0f;
		OutEffectiveStrength = 0.0f;
		OutAverageState = 0.0f;
		OutCenter = FVector::ZeroVector;
		for (const FLxAnalyzedTarget& Target : Targets)
		{
			OutBaseStrength += Target.BaseStrength;
			OutEffectiveStrength += Target.EffectiveStrength;
			OutAverageState += Target.StateRatio;
			OutCenter += Target.Character->GetActorLocation();
		}
		if (OutCount > 0)
		{
			const float CountScale = 1.0f / static_cast<float>(OutCount);
			OutAverageState *= CountScale;
			OutCenter *= CountScale;
		}
	};

	AccumulateSide(EnemyTargets, Snapshot.EnemyCount, Snapshot.EnemyBaseStrength,
		Snapshot.EnemyEffectiveStrength, Snapshot.EnemyAverageState, Snapshot.EnemyCenter);
	AccumulateSide(AssistTargets, Snapshot.AssistCount, Snapshot.AssistBaseStrength,
		Snapshot.AssistEffectiveStrength, Snapshot.AssistAverageState, Snapshot.AssistCenter);
	// 防守位置只使用实际感知到的其他友方；没有友方时留在自身位置。
	Snapshot.AssistCenter = AICharacter->GetActorLocation();
	FVector OtherAssistLocationSum = FVector::ZeroVector;
	int32 OtherAssistCount = 0;
	for (const FLxAnalyzedTarget& Assist : AssistTargets)
	{
		if (Assist.Character != AICharacter)
		{
			OtherAssistLocationSum += Assist.Character->GetActorLocation();
			++OtherAssistCount;
		}
	}
	if (OtherAssistCount > 0)
	{
		Snapshot.AssistCenter = OtherAssistLocationSum / static_cast<float>(OtherAssistCount);
	}
	Snapshot.SelfState = AICharacter->GetCurrentHealthRatio();
	Snapshot.bHasThreat = Snapshot.EnemyCount > 0;
	Snapshot.NumberAdvantageRatio = static_cast<float>(Snapshot.AssistCount) /
		static_cast<float>(FMath::Max(Snapshot.EnemyCount, 1));
	Snapshot.StrengthAdvantageRatio = Snapshot.AssistEffectiveStrength /
		FMath::Max(Snapshot.EnemyEffectiveStrength, 1.0f);
	Snapshot.StateAdvantageRatio = Snapshot.AssistAverageState / FMath::Max(Snapshot.EnemyAverageState, 0.01f);

	const float NumberComparison = CalculateNormalizedComparison(
		static_cast<float>(Snapshot.AssistCount), static_cast<float>(Snapshot.EnemyCount));
	const float StrengthComparison = CalculateNormalizedComparison(
		Snapshot.AssistEffectiveStrength, Snapshot.EnemyEffectiveStrength);
	const float StateComparison = CalculateNormalizedComparison(Snapshot.AssistAverageState, Snapshot.EnemyAverageState);
	const float TotalWeight = Config.NumberComparisonWeight + Config.StrengthComparisonWeight + Config.StateComparisonWeight;
	Snapshot.AdvantageScore = TotalWeight > UE_SMALL_NUMBER ?
		(NumberComparison * Config.NumberComparisonWeight + StrengthComparison * Config.StrengthComparisonWeight +
			StateComparison * Config.StateComparisonWeight) / TotalWeight : 0.0f;

	float HighestThreatScore = -1.0f;
	float NearestEnemyDistanceSquared = TNumericLimits<float>::Max();
	for (const FLxAnalyzedTarget& Enemy : EnemyTargets)
	{
		const float EnemyDistanceSquared = FVector::DistSquared2D(
			AICharacter->GetActorLocation(), Enemy.Character->GetActorLocation());
		const float DistanceInMeters = FMath::Max(1.0f, FMath::Sqrt(EnemyDistanceSquared) / 100.0f);
		const float ThreatScore = Enemy.EffectiveStrength / DistanceInMeters;
		if (ThreatScore > HighestThreatScore)
		{
			HighestThreatScore = ThreatScore;
			Snapshot.HighestThreatEnemy = Enemy.Character;
		}
		if (EnemyDistanceSquared < NearestEnemyDistanceSquared)
		{
			NearestEnemyDistanceSquared = EnemyDistanceSquared;
			Snapshot.NearestEnemy = Enemy.Character;
		}
	}

	for (const FLxAnalyzedTarget& Assist : AssistTargets)
	{
		if (Assist.Character != AICharacter && Assist.StateRatio < Snapshot.LowestAllyState &&
			Assist.StateRatio <= Config.InjuredAllyThreshold)
		{
			Snapshot.LowestAllyState = Assist.StateRatio;
			Snapshot.LowestStateAlly = Assist.Character;
		}
	}
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
	const ALxAICharacter* AICharacter = GetAICharacter();
	return AICharacter ? AICharacter->ResolveBaseTargetRelation(InTargetCharacter) : ELxAITargetRelation::Ignore;
}

ELxAISituationLevel ALxAIController::EvaluateSituation(const FLxAIBattleSnapshot& InSnapshot) const
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !InSnapshot.bHasThreat)
	{
		return ELxAISituationLevel::NoThreat;
	}

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	const float SituationHysteresis = FMath::Max(0.0f, Config.SituationHysteresis);
	if (InSnapshot.SelfState <= Config.SelfDangerStateThreshold ||
		(CurrentSituation == ELxAISituationLevel::SelfDanger &&
			InSnapshot.SelfState <= Config.SelfDangerStateThreshold + SituationHysteresis))
	{
		return ELxAISituationLevel::SelfDanger;
	}
	if (CurrentSituation == ELxAISituationLevel::Advantage &&
		InSnapshot.AdvantageScore >= Config.AdvantageThreshold - SituationHysteresis)
	{
		return ELxAISituationLevel::Advantage;
	}
	if (CurrentSituation == ELxAISituationLevel::Disadvantage &&
		InSnapshot.AdvantageScore <= Config.DisadvantageThreshold + SituationHysteresis)
	{
		return ELxAISituationLevel::Disadvantage;
	}
	if (InSnapshot.AdvantageScore >= Config.AdvantageThreshold)
	{
		return ELxAISituationLevel::Advantage;
	}
	if (InSnapshot.AdvantageScore <= Config.DisadvantageThreshold)
	{
		return ELxAISituationLevel::Disadvantage;
	}
	return ELxAISituationLevel::Balanced;
}

ELxAIActionType ALxAIController::SelectFirstExecutableAction(const FLxAIBattleSnapshot& InSnapshot,
	const ELxAISituationLevel InSituation, const TSet<ELxAIActionType>& InExcludedActions) const
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	const ULxAIBehaviorModule* BehaviorComponent = AICharacter ? AICharacter->GetAIBehaviorComponent() : nullptr;
	if (!AICharacter || !BehaviorComponent)
	{
		return ELxAIActionType::None;
	}

	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	const FLxAISituationBehaviorSet* BehaviorSet = Config.SituationBehaviorSets.FindByPredicate(
		[InSituation](const FLxAISituationBehaviorSet& CandidateSet)
		{
			return CandidateSet.Situation == InSituation;
		});
	if (BehaviorSet)
	{
		for (const ELxAIActionType CandidateAction : BehaviorSet->BehaviorCandidates)
		{
			// 明显劣势时，配置在防守之后的逃跑仍应先于防守尝试，避免防守的宽松条件永久遮蔽逃跑。
			if (InSituation == ELxAISituationLevel::Disadvantage && CandidateAction == ELxAIActionType::Defend &&
				BehaviorSet->BehaviorCandidates.Contains(ELxAIActionType::Retreat) &&
				!InExcludedActions.Contains(ELxAIActionType::Retreat) &&
				BehaviorComponent->CanExecuteBehavior(ELxAIActionType::Retreat, InSnapshot))
			{
				return ELxAIActionType::Retreat;
			}
			if (!InExcludedActions.Contains(CandidateAction) &&
				BehaviorComponent->CanExecuteBehavior(CandidateAction, InSnapshot))
			{
				return CandidateAction;
			}
		}
	}

	if (!InExcludedActions.Contains(Config.FallbackAction) &&
		BehaviorComponent->CanExecuteBehavior(Config.FallbackAction, InSnapshot))
	{
		return Config.FallbackAction;
	}

	// 角色配置的回退行为可能与当前局势冲突（例如有威胁时配置为巡逻），最终使用警戒保证运行中不落入“无”。
	return !InExcludedActions.Contains(ELxAIActionType::Alert) &&
		BehaviorComponent->CanExecuteBehavior(ELxAIActionType::Alert, InSnapshot) ?
		ELxAIActionType::Alert : ELxAIActionType::None;
}

void ALxAIController::SelectAndExecuteAction(const ELxAISituationLevel InSituation)
{
	ALxAICharacter* AICharacter = GetAICharacter();
	ULxAIBehaviorModule* BehaviorComponent = AICharacter ? AICharacter->GetAIBehaviorComponent() : nullptr;
	if (!BehaviorComponent)
	{
		ChangeAction(InSituation, ELxAIActionType::None);
		return;
	}

	BehaviorComponent->UpdateRetreatProgress(CurrentBattleSnapshot);
	TSet<ELxAIActionType> ExcludedActions;

	// 单次逃跑在达到配置距离前具有持续性；即使暂时丢失敌方，也会在当前路径结束后继续分段寻路。
	if (BehaviorComponent->IsRetreatInProgress())
	{
		const ELxAIBehaviorExecutionResult RetreatResult = BehaviorComponent->ExecuteBehavior(
			ELxAIActionType::Retreat, CurrentBattleSnapshot);
		if (RetreatResult != ELxAIBehaviorExecutionResult::Failed)
		{
			ChangeAction(InSituation, ELxAIActionType::Retreat);
			return;
		}
		ExcludedActions.Add(ELxAIActionType::Retreat);
	}

	// 普通行为在最短持续时间内保持执行；自身危险触发的逃跑可以立即打断它。
	const FLxAIControlConfig& Config = AICharacter->GetAIControlConfig();
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const bool bUrgentRetreat = InSituation == ELxAISituationLevel::SelfDanger &&
		CurrentAction != ELxAIActionType::Retreat &&
		BehaviorComponent->CanExecuteBehavior(ELxAIActionType::Retreat, CurrentBattleSnapshot);
	const bool bWithinMinimumDuration = CurrentAction != ELxAIActionType::None &&
		CurrentTime - CurrentActionStartTime < FMath::Max(0.0f, Config.MinimumActionDuration);
	if (!bUrgentRetreat && bWithinMinimumDuration &&
		BehaviorComponent->CanExecuteBehavior(CurrentAction, CurrentBattleSnapshot))
	{
		const ELxAIBehaviorExecutionResult CurrentResult = BehaviorComponent->ExecuteBehavior(
			CurrentAction, CurrentBattleSnapshot);
		if (CurrentResult != ELxAIBehaviorExecutionResult::Failed)
		{
			ChangeAction(InSituation, CurrentAction);
			return;
		}
		ExcludedActions.Add(CurrentAction);
	}

	while (true)
	{
		const ELxAIActionType CandidateAction = SelectFirstExecutableAction(
			CurrentBattleSnapshot, InSituation, ExcludedActions);
		if (CandidateAction == ELxAIActionType::None)
		{
			// 普通决策回退只停止当前执行，不清除逃跑完成锁，避免“逃跑→无→再逃跑”循环。
			BehaviorComponent->StopBehaviorExecution();
			ChangeAction(InSituation, ELxAIActionType::None);
			return;
		}

		if (CandidateAction != CurrentAction)
		{
			BehaviorComponent->StopBehaviorExecution();
		}
		const ELxAIBehaviorExecutionResult ExecutionResult = BehaviorComponent->ExecuteBehavior(
			CandidateAction, CurrentBattleSnapshot);
		if (ExecutionResult != ELxAIBehaviorExecutionResult::Failed)
		{
			ChangeAction(InSituation, CandidateAction);
			return;
		}

		ExcludedActions.Add(CandidateAction);
	}
}

void ALxAIController::ChangeAction(const ELxAISituationLevel InSituation, const ELxAIActionType InActionType)
{
	const bool bSituationChanged = CurrentSituation != InSituation;
	const bool bActionChanged = CurrentAction != InActionType;
	if (!bSituationChanged && !bActionChanged)
	{
		return;
	}

	CurrentSituation = InSituation;
	CurrentAction = InActionType;
	if (bActionChanged)
	{
		CurrentActionStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	}
	UE_LOG(LogTemp, Verbose, TEXT("AI决策变化：角色=%s，局势=%d，行为=%d，生命比例=%.3f，敌人数=%d。"),
		*GetNameSafe(GetPawn()), static_cast<int32>(CurrentSituation), static_cast<int32>(CurrentAction),
		CurrentBattleSnapshot.SelfState, CurrentBattleSnapshot.EnemyCount);
	OnAIActionChanged.Broadcast(CurrentSituation, CurrentAction);
}

float ALxAIController::CalculateNormalizedComparison(const float InAssistValue, const float InEnemyValue)
{
	const float SafeAssistValue = FMath::Max(0.0f, InAssistValue);
	const float SafeEnemyValue = FMath::Max(0.0f, InEnemyValue);
	const float TotalValue = SafeAssistValue + SafeEnemyValue;
	return TotalValue > UE_SMALL_NUMBER ? (SafeAssistValue - SafeEnemyValue) / TotalValue : 0.0f;
}

float ALxAIController::GetStateRatioForCharacter(const ALxBaseCharacter* InCharacter)
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

float ALxAIController::GetBaseStrengthForCharacter(const ALxBaseCharacter* InCharacter)
{
	if (!InCharacter)
	{
		return 0.0f;
	}
	const ULxCharacterAttributeComponent* AttributeComponent = InCharacter->GetCharacterAttributeComponent();
	const float TotalStrength = AttributeComponent ? static_cast<float>(AttributeComponent->CalculateTotalStrength()) : 0.0f;
	const ALxAICharacter* AICharacter = Cast<ALxAICharacter>(InCharacter);
	const float StrengthMultiplier = AICharacter ? AICharacter->GetAIControlConfig().CombatStrengthMultiplier : 1.0f;
	return FMath::Max(1.0f, TotalStrength) * FMath::Max(0.0f, StrengthMultiplier);
}

ALxAICharacter* ALxAIController::GetAICharacter() const
{
	return Cast<ALxAICharacter>(GetPawn());
}
