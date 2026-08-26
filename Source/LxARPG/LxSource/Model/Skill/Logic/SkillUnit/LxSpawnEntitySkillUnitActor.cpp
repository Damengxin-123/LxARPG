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

		// 召唤实体的命中统一转成技能单元结果，供异步创建节点继续串接后续单元。
		FLxSkillUnitResult HitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Hit, true);
		HitResult.HitTargets.Add(InTargetActor);
		HitResult.HitTargetLocations.Add(InTargetActor->GetActorLocation());
		HitResult.HitLocations.Add(InEntityActor->GetActorLocation());
		HitResult.SourceToTargetDirections.Add(
			(InTargetActor->GetActorLocation() - InEntityActor->GetActorLocation()).GetSafeNormal());
		PublishSkillUnitHitResult(HitResult);
	}
}

void ALxSpawnEntitySkillUnitActor::NotifySpawnedEntityKilledTarget(AActor* InEntityActor, AActor* InTargetActor)
{
	if (InEntityActor && InTargetActor)
	{
		OnEntityKilledTarget.Broadcast(this, InEntityActor, InTargetActor);
	}
}
