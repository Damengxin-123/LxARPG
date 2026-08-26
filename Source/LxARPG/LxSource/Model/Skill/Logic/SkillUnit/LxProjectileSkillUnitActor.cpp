#include "LxProjectileSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
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

	if (!HitTarget || !CanTriggerTarget(HitTarget))
	{
		return;
	}

	RecordTriggeredTarget(HitTarget);
	OnProjectileTriggered.Broadcast(this, MakeProjectileTriggerContext(DetectionResult));

	FLxSkillUnitResult HitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Hit, true);
	HitResult.HitTargets.Add(HitTarget);
	HitResult.HitTargetLocations.Add(HitTarget->GetActorLocation());
	HitResult.HitLocations.Add(GetActorLocation());
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
	TargetHitCounts.Reset();
	TargetLastHitTimes.Reset();
}

void ALxProjectileSkillUnitActor::HandleProjectileWorldHit(const FLxSkillDetectionResult& DetectionResult)
{
	// 障碍物碰撞属于技能命中，统一把障碍表面的碰撞点写入命中位置列表。
	FLxSkillUnitResult ObstacleHitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Blocked, true);
	ObstacleHitResult.HitLocations.Add(DetectionResult.HitLocation);
	ObstacleHitResult.HitNormals.Add(DetectionResult.HitNormal);
	PublishSkillUnitHitResult(ObstacleHitResult);
	InvalidateProjectile(MakeSpawnTransformFromDetectionResult(DetectionResult), true);
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

void ALxProjectileSkillUnitActor::InvalidateProjectile(const FTransform& InvalidationTransform, bool bHitObstacle)
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
	FLxSkillUnitResult InvalidationResult = MakeSkillUnitResult(
		bHitObstacle ? ELxSkillUnitResultType::Blocked : ELxSkillUnitResultType::Expired, true);
	FinishSkillUnit(InvalidationResult);

	// 临时投射物失效后统一销毁，创建节点不再暴露重复的生命周期开关。
	Destroy();
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

bool ALxProjectileSkillUnitActor::CanTriggerTarget(AActor* InTarget) const
{
	if (!IsValid(InTarget))
	{
		return false;
	}

	if (SkillUnitSpec.HitLimitSpec.MaxTotalHitCount > 0
		&& TriggeredTargets.Num() >= SkillUnitSpec.HitLimitSpec.MaxTotalHitCount)
	{
		return false;
	}

	const TWeakObjectPtr<AActor> TargetKey(InTarget);
	const int32 CurrentHitCount = TargetHitCounts.FindRef(TargetKey);
	if ((SkillUnitSpec.HitLimitSpec.bIgnoreAlreadyHitTargets
		|| !SkillUnitSpec.HitLimitSpec.bCanHitSameTargetAgain) && CurrentHitCount > 0)
	{
		return false;
	}
	if (SkillUnitSpec.HitLimitSpec.MaxHitCountPerTarget > 0
		&& CurrentHitCount >= SkillUnitSpec.HitLimitSpec.MaxHitCountPerTarget)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const float* LastHitTime = TargetLastHitTimes.Find(TargetKey);
	if (World && LastHitTime && SkillUnitSpec.HitLimitSpec.HitIntervalPerTarget > 0.0f
		&& World->GetTimeSeconds() - *LastHitTime < SkillUnitSpec.HitLimitSpec.HitIntervalPerTarget)
	{
		return false;
	}

	return true;
}

void ALxProjectileSkillUnitActor::RecordTriggeredTarget(AActor* InTarget)
{
	if (!IsValid(InTarget))
	{
		return;
	}

	TriggeredTargets.Add(InTarget);
	const TWeakObjectPtr<AActor> TargetKey(InTarget);
	TargetHitCounts.FindOrAdd(TargetKey) += 1;
	if (const UWorld* World = GetWorld())
	{
		TargetLastHitTimes.FindOrAdd(TargetKey) = World->GetTimeSeconds();
	}
}
