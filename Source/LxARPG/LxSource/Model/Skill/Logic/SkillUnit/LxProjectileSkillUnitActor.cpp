#include "LxProjectileSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"

ALxProjectileSkillUnitActor::ALxProjectileSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ProjectileCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
	ProjectileCollisionComponent->SetupAttachment(SceneRoot);
	ProjectileCollisionComponent->SetSphereRadius(16.0f);
	ProjectileCollisionComponent->SetGenerateOverlapEvents(true);

	MovementComponent = CreateDefaultSubobject<ULxSkillMovementComponent>(TEXT("MovementComponent"));
	DetectionComponent = CreateDefaultSubobject<ULxSkillDetectionComponent>(TEXT("DetectionComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxProjectileSkillUnitActor::InitializeProjectileParameters(const FLxSkillProjectileSpec& InProjectileSpec)
{
	ProjectileSpec = InProjectileSpec;
	SkillUnitSpec.MovementSpec.Speed = ProjectileSpec.FlightSpeed;
	SkillUnitSpec.MovementSpec.Acceleration = ProjectileSpec.FlightAcceleration;
	SkillUnitSpec.MovementSpec.MaxDistance = ProjectileSpec.MaxFlightDistance;

	if (MovementComponent)
	{
		MovementComponent->SetMovementSpec(SkillUnitSpec.MovementSpec);
	}
}

void ALxProjectileSkillUnitActor::ActivateSkillUnit_Implementation()
{
	ResetProjectileRuntimeState();
	Super::ActivateSkillUnit_Implementation();
}

void ALxProjectileSkillUnitActor::InitializeSkillUnitDefaultParameters_Implementation()
{
	Super::InitializeSkillUnitDefaultParameters_Implementation();

	if (MovementComponent)
	{
		MovementComponent->SetMovementTargetComponent(GetRootComponent());
	}

	if (DetectionComponent)
	{
		DetectionComponent->SetTriggerCollisionComponent(ProjectileCollisionComponent);
		DetectionComponent->SetPublishWorldHit(true);
	}
}

void ALxProjectileSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();
}

void ALxProjectileSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();

	if (DetectionComponent)
	{
		DetectionComponent->OnDetectionResult.AddUniqueDynamic(this, &ALxProjectileSkillUnitActor::HandleProjectileDetectionResult);
	}

	if (MovementComponent)
	{
		MovementComponent->OnReachMaxDistance.AddUniqueDynamic(this, &ALxProjectileSkillUnitActor::HandleProjectileReachMaxDistance);
	}
}

void ALxProjectileSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		InvalidateProjectile(GetActorTransform());
		return;
	}

	Super::HandleLifeStateChanged(OldState, NewState);
}

void ALxProjectileSkillUnitActor::HandleProjectileDetectionResult(const FLxSkillDetectionResult& DetectionResult)
{
	if (bProjectileInvalidated)
	{
		return;
	}

	if (DetectionResult.EventType == ELxSkillDetectionEventType::HitWorld || DetectionResult.bHitWorld)
	{
		HandleProjectileWorldHit(DetectionResult);
		return;
	}

	if (DetectionResult.EventType != ELxSkillDetectionEventType::HitTarget && DetectionResult.EventType != ELxSkillDetectionEventType::OverlapBegin)
	{
		return;
	}

	AActor* HitTarget = DetectionResult.HitActor;
	if (!HitTarget && DetectionResult.CandidateTargets.Num() > 0)
	{
		HitTarget = DetectionResult.CandidateTargets[0];
	}

	if (!HitTarget || HasTriggeredTarget(HitTarget))
	{
		return;
	}

	TriggeredTargets.Add(HitTarget);
	OnProjectileTriggered.Broadcast(this, MakeProjectileTriggerContext(DetectionResult));

	FLxSkillUnitResult HitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Hit, true);
	HitResult.HitTargets.Add(HitTarget);
	HitResult.HitLocations.Add(DetectionResult.HitLocation);
	HitResult.HitNormals.Add(DetectionResult.HitNormal);
	HitResult.SourceToTargetDirections.Add((HitTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal());
	HitResult.TriggeredCount = TriggeredTargets.Num();
	PublishSkillUnitHitResult(HitResult);

	if (RemainingPierceCount > 0)
	{
		--RemainingPierceCount;
		return;
	}

	InvalidateProjectile(MakeSpawnTransformFromDetectionResult(DetectionResult));
}

void ALxProjectileSkillUnitActor::HandleProjectileReachMaxDistance(float MovementProgress)
{
	InvalidateProjectile(GetActorTransform());
}

void ALxProjectileSkillUnitActor::ResetProjectileRuntimeState()
{
	RemainingPierceCount = FMath::Max(ProjectileSpec.MaxPierceCount, 0);
	bProjectileInvalidated = false;
	TriggeredTargets.Reset();
}

void ALxProjectileSkillUnitActor::HandleProjectileWorldHit(const FLxSkillDetectionResult& DetectionResult)
{
	InvalidateProjectile(MakeSpawnTransformFromDetectionResult(DetectionResult));
}

FLxProjectileTriggerContext ALxProjectileSkillUnitActor::MakeProjectileTriggerContext(const FLxSkillDetectionResult& DetectionResult) const
{
	FLxProjectileTriggerContext TriggerContext;
	TriggerContext.HitTarget = DetectionResult.HitActor;
	if (!TriggerContext.HitTarget && DetectionResult.CandidateTargets.Num() > 0)
	{
		TriggerContext.HitTarget = DetectionResult.CandidateTargets[0];
	}
	TriggerContext.SpawnTransform = MakeSpawnTransformFromDetectionResult(DetectionResult);
	return TriggerContext;
}

FLxProjectileInvalidationContext ALxProjectileSkillUnitActor::MakeProjectileInvalidationContext(const FTransform& InvalidationTransform) const
{
	FLxProjectileInvalidationContext InvalidationContext;
	InvalidationContext.SpawnTransform = InvalidationTransform;
	return InvalidationContext;
}

void ALxProjectileSkillUnitActor::InvalidateProjectile(const FTransform& InvalidationTransform)
{
	if (bProjectileInvalidated)
	{
		return;
	}

	bProjectileInvalidated = true;

	if (MovementComponent)
	{
		MovementComponent->StopMovement();
	}

	if (DetectionComponent)
	{
		DetectionComponent->StopDetection();
	}

	if (LifeComponent)
	{
		LifeComponent->StopLife();
	}

	OnProjectileInvalidated.Broadcast(this, MakeProjectileInvalidationContext(InvalidationTransform));
	FinishSkillUnit(MakeSkillUnitResult(ELxSkillUnitResultType::Expired, true));

	if (ProjectileSpec.bDestroyAfterInvalidated)
	{
		Destroy();
	}
}

FTransform ALxProjectileSkillUnitActor::MakeSpawnTransformFromDetectionResult(const FLxSkillDetectionResult& DetectionResult) const
{
	FVector SpawnLocation = DetectionResult.HitLocation;
	if (SpawnLocation.IsNearlyZero())
	{
		if (DetectionResult.HitActor)
		{
			SpawnLocation = DetectionResult.HitActor->GetActorLocation();
		}
		else
		{
			SpawnLocation = GetActorLocation();
		}
	}

	return FTransform(GetActorRotation(), SpawnLocation);
}

bool ALxProjectileSkillUnitActor::HasTriggeredTarget(AActor* InTarget) const
{
	for (AActor* TriggeredTarget : TriggeredTargets)
	{
		if (TriggeredTarget == InTarget)
		{
			return true;
		}
	}

	return false;
}
