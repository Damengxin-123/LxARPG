#include "LxSpawnEntitySkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"

ALxSpawnEntitySkillUnitActor::ALxSpawnEntitySkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxSpawnEntitySkillUnitActor::InitializeSpawnEntityParameters(const FLxSkillSpawnEntitySpec& InSpawnEntitySpec)
{
	SpawnEntitySpec = InSpawnEntitySpec;
}

void ALxSpawnEntitySkillUnitActor::RegisterSpawnedEntity(AActor* InEntityActor)
{
	if (!InEntityActor)
	{
		return;
	}

	SpawnedEntities.AddUnique(InEntityActor);
	OnEntityCreated.Broadcast(this, InEntityActor);
}

void ALxSpawnEntitySkillUnitActor::NotifySpawnedEntityDestroyed(AActor* InEntityActor)
{
	if (!InEntityActor)
	{
		return;
	}

	SpawnedEntities.Remove(InEntityActor);
	OnEntityDestroyed.Broadcast(this, InEntityActor);
}

void ALxSpawnEntitySkillUnitActor::NotifySpawnedEntityHitTarget(AActor* InEntityActor, AActor* InTargetActor)
{
	if (InEntityActor && InTargetActor)
	{
		OnEntityHitTarget.Broadcast(this, InEntityActor, InTargetActor);
	}
}

void ALxSpawnEntitySkillUnitActor::NotifySpawnedEntityKilledTarget(AActor* InEntityActor, AActor* InTargetActor)
{
	if (InEntityActor && InTargetActor)
	{
		OnEntityKilledTarget.Broadcast(this, InEntityActor, InTargetActor);
	}
}
