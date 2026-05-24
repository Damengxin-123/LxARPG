#include "LxProjectileSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillPropagationComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

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
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	PropagationComponent = CreateDefaultSubobject<ULxSkillPropagationComponent>(TEXT("PropagationComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxProjectileSkillUnitActor::InitializeProjectileParameters(const FLxSkillProjectileSpec& InProjectileSpec)
{
	ProjectileSpec = InProjectileSpec;
	if (ProjectileCollisionComponent)
	{
		ProjectileCollisionComponent->SetSphereRadius(ProjectileSpec.CollisionRadius);
	}
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
	}
}

void ALxProjectileSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

	if (ProjectileCollisionComponent && SkillUnitSpec.SpaceSpec.Radius > 0.0f)
	{
		ProjectileCollisionComponent->SetSphereRadius(SkillUnitSpec.SpaceSpec.Radius);
	}
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

	if (PropagationComponent)
	{
		PropagationComponent->OnPropagationEvaluated.AddUniqueDynamic(this, &ALxProjectileSkillUnitActor::HandleProjectilePropagationResult);
	}
}

void ALxProjectileSkillUnitActor::HandleProjectileDetectionResult(const FLxSkillDetectionResult& DetectionResult)
{
	if (DetectionResult.EventType == ELxSkillDetectionEventType::HitTarget || DetectionResult.EventType == ELxSkillDetectionEventType::OverlapBegin)
	{
		OnProjectileHitTarget.Broadcast(DetectionResult);
	}
	else if (DetectionResult.EventType == ELxSkillDetectionEventType::HitWorld)
	{
		OnProjectileHitWorld.Broadcast(DetectionResult);
	}
}

void ALxProjectileSkillUnitActor::HandleProjectilePropagationResult(const FLxSkillPropagationResult& PropagationResult)
{
	if (PropagationResult.DecisionType == ELxSkillPropagationDecisionType::Pierce)
	{
		OnProjectilePierceTarget.Broadcast(PropagationResult);
	}
}

void ALxProjectileSkillUnitActor::HandleProjectileReachMaxDistance(float MovementProgress)
{
	OnProjectileReachMaxDistance.Broadcast(MovementProgress);
}
