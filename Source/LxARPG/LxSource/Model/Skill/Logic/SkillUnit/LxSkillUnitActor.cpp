#include "LxSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillPropagationComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxSkillUnitActor::ALxSkillUnitActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALxSkillUnitActor::BeginPlay()
{
	Super::BeginPlay();

	InitializeSkillUnitDefaultParameters();
	BindSkillUnitComponentEvents();

	if (bAutoActivateSkillUnit)
	{
		ActivateSkillUnit();
	}
}

void ALxSkillUnitActor::InitializeSkillUnit(const FLxSkillUnitSpec& InSkillUnitSpec)
{
	SkillUnitSpec = InSkillUnitSpec;
	ApplySkillUnitSpecToComponents();
	BindSkillUnitComponentEvents();
	bSkillUnitInitialized = true;
}

void ALxSkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (!bSkillUnitInitialized)
	{
		ApplySkillUnitSpecToComponents();
		BindSkillUnitComponentEvents();
		bSkillUnitInitialized = true;
	}

	bSkillUnitActive = true;

	if (ULxSkillLifeComponent* LifeComponent = GetSkillLifeComponent())
	{
		LifeComponent->StartLife();
	}

	if (ULxSkillDetectionComponent* DetectionComponent = GetSkillDetectionComponent())
	{
		DetectionComponent->StartDetection();
	}

	if (ULxSkillTriggerComponent* TriggerComponent = GetSkillTriggerComponent())
	{
		TriggerComponent->StartTrigger();
	}

	if (ULxSkillMovementComponent* MovementComponent = GetSkillMovementComponent())
	{
		MovementComponent->StartMovement();
	}

	OnSkillUnitActivated.Broadcast(this, MakeSkillUnitResult(ELxSkillUnitResultType::Started, true));
}

void ALxSkillUnitActor::CancelSkillUnit_Implementation()
{
	bSkillUnitActive = false;

	if (ULxSkillMovementComponent* MovementComponent = GetSkillMovementComponent())
	{
		MovementComponent->StopMovement();
	}

	if (ULxSkillDetectionComponent* DetectionComponent = GetSkillDetectionComponent())
	{
		DetectionComponent->StopDetection();
	}

	if (ULxSkillTriggerComponent* TriggerComponent = GetSkillTriggerComponent())
	{
		TriggerComponent->StopTrigger();
	}

	if (ULxSkillLifeComponent* LifeComponent = GetSkillLifeComponent())
	{
		LifeComponent->StopLife();
	}

	const FLxSkillUnitResult Result = MakeSkillUnitResult(ELxSkillUnitResultType::Cancelled, false);
	OnSkillUnitCancelled.Broadcast(this, Result);
	OnSkillUnitFinished.Broadcast(this, Result);
}

void ALxSkillUnitActor::SetSkillUnitSpawnSpec(const FLxSkillUnitSpawnSpec& InSpawnSpec)
{
	SkillUnitSpec.SpawnSpec = InSpawnSpec;
}

void ALxSkillUnitActor::SetSkillUnitEntryPackages(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages)
{
	SkillEntryPackages = InSkillEntryPackages;
}
void ALxSkillUnitActor::InitializeSkillUnitParameterLimits_Implementation()
{
}

void ALxSkillUnitActor::InitializeSkillUnitDefaultParameters_Implementation()
{
}

ULxSkillMovementComponent* ALxSkillUnitActor::GetSkillMovementComponent() const
{
	return FindComponentByClass<ULxSkillMovementComponent>();
}

ULxSkillDetectionComponent* ALxSkillUnitActor::GetSkillDetectionComponent() const
{
	return FindComponentByClass<ULxSkillDetectionComponent>();
}

ULxSkillLifeComponent* ALxSkillUnitActor::GetSkillLifeComponent() const
{
	return FindComponentByClass<ULxSkillLifeComponent>();
}

ULxSkillTriggerComponent* ALxSkillUnitActor::GetSkillTriggerComponent() const
{
	return FindComponentByClass<ULxSkillTriggerComponent>();
}

ULxSkillPropagationComponent* ALxSkillUnitActor::GetSkillPropagationComponent() const
{
	return FindComponentByClass<ULxSkillPropagationComponent>();
}

void ALxSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	if (ULxSkillMovementComponent* MovementComponent = GetSkillMovementComponent())
	{
		MovementComponent->SetMovementSpec(SkillUnitSpec.MovementSpec);
	}

	if (ULxSkillLifeComponent* LifeComponent = GetSkillLifeComponent())
	{
		LifeComponent->SetLifeSpec(SkillUnitSpec.LifeSpec);
		LifeComponent->SetLifeTickInterval(SkillUnitSpec.TriggerSpec.TickInterval);
	}

	if (ULxSkillDetectionComponent* DetectionComponent = GetSkillDetectionComponent())
	{
		DetectionComponent->SetTargetFilterSpec(SkillUnitSpec.TargetFilterSpec);
	}

	if (ULxSkillTriggerComponent* TriggerComponent = GetSkillTriggerComponent())
	{
		TriggerComponent->SetTriggerSpec(SkillUnitSpec.TriggerSpec);
		TriggerComponent->SetTargetFilterSpec(SkillUnitSpec.TargetFilterSpec);
		TriggerComponent->SetHitLimitSpec(SkillUnitSpec.HitLimitSpec);
	}

	if (ULxSkillPropagationComponent* PropagationComponent = GetSkillPropagationComponent())
	{
		PropagationComponent->SetPropagationSpec(SkillUnitSpec.PropagationSpec);
	}
}

void ALxSkillUnitActor::BindSkillUnitComponentEvents()
{
	if (ULxSkillLifeComponent* LifeComponent = GetSkillLifeComponent())
	{
		LifeComponent->OnLifeStateChanged.AddUniqueDynamic(this, &ALxSkillUnitActor::HandleLifeStateChanged);
	}

	if (ULxSkillTriggerComponent* TriggerComponent = GetSkillTriggerComponent())
	{
		TriggerComponent->OnTriggered.AddUniqueDynamic(this, &ALxSkillUnitActor::HandleSkillTriggered);
	}

	if (ULxSkillPropagationComponent* PropagationComponent = GetSkillPropagationComponent())
	{
		PropagationComponent->OnPropagationEvaluated.AddUniqueDynamic(this, &ALxSkillUnitActor::HandlePropagationEvaluated);
	}
}

void ALxSkillUnitActor::FinishSkillUnit(const FLxSkillUnitResult& InResult)
{
	bSkillUnitActive = false;
	OnSkillUnitFinished.Broadcast(this, InResult);
}

FLxSkillUnitResult ALxSkillUnitActor::MakeSkillUnitResult(ELxSkillUnitResultType InResultType, bool bSuccess) const
{
	FLxSkillUnitResult Result;
	Result.bSuccess = bSuccess;
	Result.ResultType = InResultType;
	Result.EndLocation = GetActorLocation();
	Result.EndRotation = GetActorRotation();
	Result.SourceUnit = const_cast<ALxSkillUnitActor*>(this);
	return Result;
}

void ALxSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		FinishSkillUnit(MakeSkillUnitResult(ELxSkillUnitResultType::Expired, true));
	}
}

void ALxSkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	OnSkillUnitTriggered.Broadcast(this, TriggerResult);

	if (ULxSkillPropagationComponent* PropagationComponent = GetSkillPropagationComponent())
	{
		PropagationComponent->EvaluatePropagation(TriggerResult);
	}
}

void ALxSkillUnitActor::HandlePropagationEvaluated(const FLxSkillPropagationResult& PropagationResult)
{
	OnSkillUnitPropagationEvaluated.Broadcast(this, PropagationResult);
}
