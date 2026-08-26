#include "LxSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillPropagationComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "Net/UnrealNetwork.h"

ALxSkillUnitActor::ALxSkillUnitActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	bNetLoadOnClient = true;
}

void ALxSkillUnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALxSkillUnitActor, SkillUnitSpec);
	DOREPLIFETIME(ALxSkillUnitActor, bSkillUnitActive);
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
	if (bSkillUnitActive)
	{
		return;
	}

	if (!bSkillUnitInitialized)
	{
		ApplySkillUnitSpecToComponents();
		BindSkillUnitComponentEvents();
		bSkillUnitInitialized = true;
	}

	bSkillUnitActive = true;

	// 客户端只复现蓝图表现和激活事件，所有权威组件逻辑仅由服务端启动。
	if (!HasAuthority())
	{
		OnSkillUnitActivated.Broadcast(this, MakeSkillUnitResult(ELxSkillUnitResultType::Started, true));
		return;
	}

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

void ALxSkillUnitActor::StopSkillUnit_Implementation()
{
	if (!bSkillUnitActive)
	{
		return;
	}

	bSkillUnitActive = false;
	StopSkillUnitComponents();
	OnSkillUnitFinished.Broadcast(this, MakeSkillUnitResult(ELxSkillUnitResultType::Completed, true));
}

void ALxSkillUnitActor::CancelSkillUnit_Implementation()
{
	if (!bSkillUnitActive)
	{
		return;
	}

	bSkillUnitActive = false;
	StopSkillUnitComponents();

	const FLxSkillUnitResult Result = MakeSkillUnitResult(ELxSkillUnitResultType::Cancelled, false);
	OnSkillUnitCancelled.Broadcast(this, Result);
	OnSkillUnitFinished.Broadcast(this, Result);
}

void ALxSkillUnitActor::UpdateSkillUnitTransform_Implementation(const FTransform& InTransform)
{
	SetActorTransform(InTransform);
}

void ALxSkillUnitActor::StopSkillUnitComponents()
{
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
void ALxSkillUnitActor::PublishSkillUnitHitResult(const FLxSkillUnitResult& HitResult)
{
	const bool bIsHitResult = HitResult.ResultType == ELxSkillUnitResultType::Hit
		|| HitResult.ResultType == ELxSkillUnitResultType::Blocked;
	if (HasAuthority() && HitResult.bSuccess && bIsHitResult
		&& (!HitResult.HitTargets.IsEmpty() || !HitResult.HitLocations.IsEmpty()))
	{
		OnSkillUnitHit.Broadcast(this, HitResult);
	}
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
	if (!HasAuthority())
	{
		return;
	}

	OnSkillUnitTriggered.Broadcast(this, TriggerResult);

	if (TriggerResult.bTriggered && !TriggerResult.TriggeredTargets.IsEmpty())
	{
		FLxSkillUnitResult HitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Hit, true);
		for (AActor* TriggeredTarget : TriggerResult.TriggeredTargets)
		{
			if (!IsValid(TriggeredTarget))
			{
				continue;
			}
			const FVector TargetLocation = TriggeredTarget->GetActorLocation();
			HitResult.HitTargets.Add(TriggeredTarget);
			HitResult.HitTargetLocations.Add(TargetLocation);
			HitResult.HitLocations.Add(GetActorLocation());
			HitResult.HitNormals.Add(TriggerResult.DetectionResult.HitNormal);
			HitResult.SourceToTargetDirections.Add((TargetLocation - GetActorLocation()).GetSafeNormal());
		}
		HitResult.TriggeredCount = TriggerResult.TriggeredCount;
		PublishSkillUnitHitResult(HitResult);
	}

	if (ULxSkillPropagationComponent* PropagationComponent = GetSkillPropagationComponent())
	{
		PropagationComponent->EvaluatePropagation(TriggerResult);
	}
}

void ALxSkillUnitActor::HandlePropagationEvaluated(const FLxSkillPropagationResult& PropagationResult)
{
	if (!HasAuthority())
	{
		return;
	}
	OnSkillUnitPropagationEvaluated.Broadcast(this, PropagationResult);
}

void ALxSkillUnitActor::OnRep_SkillUnitSpec()
{
	ApplySkillUnitSpecToComponents();
	BindSkillUnitComponentEvents();
	bSkillUnitInitialized = true;
}

void ALxSkillUnitActor::OnRep_SkillUnitActive(bool bOldSkillUnitActive)
{
	if (bSkillUnitActive == bOldSkillUnitActive)
	{
		return;
	}

	// 先恢复复制前的旧状态，使激活和停止函数的状态保护能够正常进入。
	bSkillUnitActive = bOldSkillUnitActive;
	if (!bOldSkillUnitActive)
	{
		ActivateSkillUnit();
	}
	else
	{
		StopSkillUnit();
	}
}
