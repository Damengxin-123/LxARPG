#include "LxAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "Engine/GameInstance.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorTreeExecutor.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace { constexpr float MetersToCentimeters = 100.0f; }

ALxAIController::ALxAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ALxAIController::HandleTargetPerceptionUpdated);
}

void ALxAIController::OnPossess(APawn* InPawn)
{
	ResetPerceptionForPawnChange();
	Super::OnPossess(InPawn);
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter) return;
	TargetMemory.Reset();
	DynamicHostileTargets.Reset();
	LastAnalysisTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ULxAIBehaviorTreeAsset* Asset = AICharacter->GetAIBehaviorTreeAsset();
	if (!Asset) return;
	FText Error;
	if (!Asset->ValidateConfiguration(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("AI行为树无效，已禁用自动控制：%s：%s"), *GetNameSafe(AICharacter), *Error.ToString());
		return;
	}
	ApplyPerceptionConfiguration();
	AnalysisSession = NewObject<ULxAIControlAnalysis>(this);
	AnalysisSession->Initialize(Asset);
	BehaviorTreeExecutor = NewObject<ULxAIBehaviorTreeExecutor>(this);
	BehaviorTreeExecutor->Initialize(AICharacter);
	BehaviorTreeExecutor->OnLeafChanged.BindUObject(this, &ALxAIController::HandleLeafChanged);
	if (AICharacter->IsAIAutomaticControlEnabled())
	{
		RunAnalysisDecision();
		GetWorldTimerManager().SetTimer(AutomaticDecisionTimer, this, &ALxAIController::RunAnalysisDecision, 0.1f, true);
	}
}

void ALxAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(AutomaticDecisionTimer);
	if (BehaviorTreeExecutor) BehaviorTreeExecutor->ResetExecution();
	BehaviorTreeExecutor = nullptr;
	AnalysisSession = nullptr;
	TargetMemory.Reset();
	DynamicHostileTargets.Reset();
	LastAnalysisTime = 0.0;
	Super::OnUnPossess();
	ResetPerceptionForPawnChange();
}

void ALxAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutomaticDecisionTimer);
	if (BehaviorTreeExecutor) BehaviorTreeExecutor->ResetExecution();
	BehaviorTreeExecutor = nullptr;
	AnalysisSession = nullptr;
	TargetMemory.Reset();
	DynamicHostileTargets.Reset();
	ResetPerceptionForPawnChange();
	Super::EndPlay(EndPlayReason);
}

void ALxAIController::NotifyReceivedAttack()
{
	if (!AnalysisSession) return;
	LastAnalysisTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAnalysisTime;
	AnalysisSession->NotifyAttacked();
	RunAnalysisDecision();
}

void ALxAIController::CompleteAttackedResponse()
{
	if (!AnalysisSession) return;
	AnalysisSession->CompleteAttackedResponse();
	RunAnalysisDecision();
}

FLxAIAnalysisDecision ALxAIController::GetCurrentAnalysisDecision() const
{
	return AnalysisSession ? AnalysisSession->GetCurrentDecision() : FLxAIAnalysisDecision();
}

ELxAIBehaviorAction ALxAIController::GetCurrentBehaviorAction() const
{
	return BehaviorTreeExecutor ? BehaviorTreeExecutor->GetCurrentAction() : ELxAIBehaviorAction::Wait;
}

FGuid ALxAIController::GetCurrentBehaviorActionNodeId() const
{
	return BehaviorTreeExecutor ? BehaviorTreeExecutor->GetCurrentActionNodeId() : FGuid();
}

void ALxAIController::SuspendForMainMenu()
{
	if (BehaviorTreeExecutor) BehaviorTreeExecutor->ResetExecution();
	StopMovement();
	TargetMemory.Reset();
	DynamicHostileTargets.Reset();
	LastAnalysisTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void ALxAIController::RunAnalysisDecision()
{
	const ALxARPGGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ALxARPGGameMode>() : nullptr;
	const ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if ((Mode && Mode->IsShowingMainMenu()) || (Menu && Menu->IsEnteringGame()))
	{
		LastAnalysisTime = GetWorld()->GetTimeSeconds();
		return;
	}
	ALxAICharacter* AICharacter = GetAICharacter();
	if (!AnalysisSession || !BehaviorTreeExecutor || !AICharacter || !AICharacter->HasAuthority() ||
		!AICharacter->IsAIAutomaticControlEnabled()) return;
	RefreshActivePerceptionMemory();
	PruneTargetMemory();
	AActor* NearestEnemy = nullptr;
	FVector NearestEnemyLocation = FVector::ZeroVector;
	const FVector SelfLocation = AICharacter->GetActorLocation();
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (const TPair<TWeakObjectPtr<AActor>, FLxAITargetMemoryRecord>& Pair : TargetMemory)
	{
		if (ResolveTargetRelation(Pair.Value.TargetCharacter.Get()) != ELxAITargetRelation::Hostile) continue;
		const FVector Location = GetLatestRememberedLocation(Pair.Value);
		const double DistanceSquared = FVector::DistSquared(SelfLocation, Location);
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestEnemy = Pair.Key.Get();
			NearestEnemyLocation = Location;
			NearestDistanceSquared = DistanceSquared;
		}
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAnalysisTime;
	const FLxAIAnalysisDecision OldDecision = AnalysisSession->GetCurrentDecision();
	AnalysisSession->SetSelfLocation(SelfLocation);
	const FLxAIAnalysisDecision NewDecision = AnalysisSession->Evaluate(NearestEnemy,
		NearestEnemy ? static_cast<float>(FMath::Sqrt(NearestDistanceSquared) / MetersToCentimeters) : 0.0f,
		AICharacter->GetCurrentHealthRatio(),
		FMath::Max(0.0f, static_cast<float>(Now - LastAnalysisTime)), BehaviorTreeExecutor->CanInterruptCurrentAction());
	LastAnalysisTime = Now;
	if (BehaviorTreeExecutor->TickExecution(AICharacter->GetAIBehaviorTreeAsset(), NewDecision,
		NearestEnemy, NearestEnemyLocation, 0.1f))
	{
		AnalysisSession->CompleteAttackedResponse();
	}
	if (AnalysisSession->IsReturningFromChase() && BehaviorTreeExecutor->HasReachedPatrolDestinationThisTick())
		AnalysisSession->CompleteChaseReturn();
	if (NewDecision.Entry == ELxAIBehaviorEntry::CharacterDeath && AICharacter->GetCurrentHealthRatio() <= 0.0f
		&& AICharacter->GetCharacterAttributeComponent() && !AICharacter->GetCharacterAttributeComponent()->IsCharacterAlive())
	{
		AnalysisSession->CompleteCharacterDeath();
		GetWorldTimerManager().ClearTimer(AutomaticDecisionTimer);
	}
	if (NewDecision.bStartNewBehavior || OldDecision.bHasBranch != NewDecision.bHasBranch ||
		OldDecision.EntryId != NewDecision.EntryId || OldDecision.StateId != NewDecision.StateId ||
		OldDecision.PhaseId != NewDecision.PhaseId)
	{
		OnAIAnalysisDecisionChanged.Broadcast(NewDecision);
	}
}

void ALxAIController::HandleLeafChanged(const ELxAIBehaviorAction InAction)
{
	OnAIBehaviorActionChanged.Broadcast(InAction);
}

void ALxAIController::ResetPerceptionForPawnChange()
{
	if (!AIPerceptionComponent) return;
	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALxAIController::HandleTargetPerceptionUpdated);
	AIPerceptionComponent->ProcessStimuli();
	AIPerceptionComponent->ForgetAll();
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ALxAIController::HandleTargetPerceptionUpdated);
}

void ALxAIController::ReportPerceivedTarget(AActor* InTargetActor, const ELxAIPerceptionSource InSource, const bool bHostile)
{
	StorePerceivedTarget(InTargetActor, InSource, IsValid(InTargetActor) ? InTargetActor->GetActorLocation() : FVector::ZeroVector, bHostile);
}

void ALxAIController::StorePerceivedTarget(AActor* InActor, const ELxAIPerceptionSource InSource,
	const FVector& InLocation, const bool bHostile)
{
	ALxBaseCharacter* Target = Cast<ALxBaseCharacter>(InActor);
	const ALxAICharacter* Self = GetAICharacter();
	if (!IsValid(Target) || !Self || Target == Self || !AnalysisSession) return;
	const FLxAIPerceptionConfig& Config = Self->GetAIBehaviorTreeAsset()->Perception;
	if (InSource == ELxAIPerceptionSource::Sight || InSource == ELxAIPerceptionSource::Hearing)
	{
		if ((InSource == ELxAIPerceptionSource::Sight && !Config.bEnableSight) ||
			(InSource == ELxAIPerceptionSource::Hearing && !Config.bEnableHearing)) return;
		const ELxAITargetRelation Relation = ResolveTargetRelation(Target);
		if ((Relation == ELxAITargetRelation::Hostile && !Config.bDetectEnemies) ||
			(Relation == ELxAITargetRelation::Assist && !Config.bDetectFriendlies) ||
			(Relation == ELxAITargetRelation::Ignore && !Config.bDetectNeutrals)) return;
	}
	FLxAITargetMemoryRecord& Record = TargetMemory.FindOrAdd(InActor);
	Record.TargetCharacter = Target;
	const double SensedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (bHostile) DynamicHostileTargets.Add(InActor);
	if (InSource == ELxAIPerceptionSource::Sight) { Record.SightLocation = InLocation; Record.SightTime = SensedTime; Record.bHasSight = true; }
	else if (InSource == ELxAIPerceptionSource::Hearing) { Record.HearingLocation = InLocation; Record.HearingTime = SensedTime; Record.bHasHearing = true; }
	else { Record.OtherLocation = InLocation; Record.OtherTime = SensedTime; Record.bHasOther = true; }
}

void ALxAIController::HandleTargetPerceptionUpdated(AActor* InActor, FAIStimulus InStimulus)
{
	if (!InStimulus.WasSuccessfullySensed()) return;
	ELxAIPerceptionSource Source = ELxAIPerceptionSource::Unknown;
	bool bHostile = false;
	if (InStimulus.Type == UAISense::GetSenseID<UAISense_Damage>()) { Source = ELxAIPerceptionSource::Damage; bHostile = true; }
	else if (InStimulus.Type == UAISense::GetSenseID<UAISense_Sight>()) Source = ELxAIPerceptionSource::Sight;
	else if (InStimulus.Type == UAISense::GetSenseID<UAISense_Hearing>()) Source = ELxAIPerceptionSource::Hearing;
	StorePerceivedTarget(InActor, Source, InStimulus.StimulusLocation, bHostile);
}

void ALxAIController::ApplyPerceptionConfiguration()
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !AICharacter->GetAIBehaviorTreeAsset()) return;
	const FLxAIPerceptionConfig& Config = AICharacter->GetAIBehaviorTreeAsset()->Perception;
	SightConfig->SightRadius = Config.SightRadiusMeters * MetersToCentimeters;
	SightConfig->LoseSightRadius = Config.LoseSightRadiusMeters * MetersToCentimeters;
	SightConfig->PeripheralVisionAngleDegrees = Config.SightHalfAngleDegrees;
	SightConfig->SetMaxAge(Config.SightMemorySeconds);
	HearingConfig->HearingRange = Config.HearingRadiusMeters * MetersToCentimeters;
	HearingConfig->SetMaxAge(Config.HearingMemorySeconds);
	DamageConfig->SetMaxAge(AICharacter->GetAIBehaviorTreeAsset()->Analysis.AttackedAlertSeconds);
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->SetSenseEnabled(UAISense_Sight::StaticClass(), Config.bEnableSight);
	AIPerceptionComponent->SetSenseEnabled(UAISense_Hearing::StaticClass(), Config.bEnableHearing);
	AIPerceptionComponent->RequestStimuliListenerUpdate();
}

void ALxAIController::RefreshActivePerceptionMemory()
{
	TArray<AActor*> Actors;
	AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Actors);
	for (AActor* Actor : Actors) ReportPerceivedTarget(Actor, ELxAIPerceptionSource::Sight);
	Actors.Reset();
	AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), Actors);
	for (AActor* Actor : Actors) ReportPerceivedTarget(Actor, ELxAIPerceptionSource::Hearing);
}

void ALxAIController::PruneTargetMemory()
{
	const ALxAICharacter* AICharacter = GetAICharacter();
	if (!AICharacter || !AICharacter->GetAIBehaviorTreeAsset()) return;
	const FLxAIPerceptionConfig& Config = AICharacter->GetAIBehaviorTreeAsset()->Perception;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	for (auto It = TargetMemory.CreateIterator(); It; ++It)
	{
		FLxAITargetMemoryRecord& Record = It.Value();
		Record.bHasSight &= Config.bEnableSight && (Config.SightMemorySeconds == 0.0f || Now - Record.SightTime <= Config.SightMemorySeconds);
		Record.bHasHearing &= Config.bEnableHearing && (Config.HearingMemorySeconds == 0.0f || Now - Record.HearingTime <= Config.HearingMemorySeconds);
		Record.bHasOther &= Now - Record.OtherTime <= AICharacter->GetAIBehaviorTreeAsset()->Analysis.AttackedAlertSeconds;
		if (!Record.TargetCharacter.IsValid() || (!Record.bHasSight && !Record.bHasHearing && !Record.bHasOther))
		{
			DynamicHostileTargets.Remove(It.Key());
			It.RemoveCurrent();
		}
	}
}

ELxAITargetRelation ALxAIController::ResolveTargetRelation(const ALxBaseCharacter* InTarget) const
{
	if (!InTarget) return ELxAITargetRelation::Ignore;
	if (DynamicHostileTargets.Contains(InTarget)) return ELxAITargetRelation::Hostile;
	const ALxAICharacter* Self = GetAICharacter();
	return Self ? Self->ResolveBaseTargetRelation(InTarget) : ELxAITargetRelation::Ignore;
}

FVector ALxAIController::GetLatestRememberedLocation(const FLxAITargetMemoryRecord& Record) const
{
	if (Record.bHasSight && (!Record.bHasHearing || Record.SightTime >= Record.HearingTime) && (!Record.bHasOther || Record.SightTime >= Record.OtherTime)) return Record.SightLocation;
	if (Record.bHasHearing && (!Record.bHasOther || Record.HearingTime >= Record.OtherTime)) return Record.HearingLocation;
	return Record.OtherLocation;
}

ALxAICharacter* ALxAIController::GetAICharacter() const
{
	return Cast<ALxAICharacter>(GetPawn());
}
