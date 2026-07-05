#include "LxAreaSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxAreaSkillUnitActor::ALxAreaSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DetectionComponent = CreateDefaultSubobject<ULxSkillDetectionComponent>(TEXT("DetectionComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxAreaSkillUnitActor::InitializeAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec)
{
	AreaEffectSpec = InAreaEffectSpec;
	DetectionCollisionComponents.Reset();
	for (UPrimitiveComponent* CollisionComponent : ResolveAreaDetectionCollisionComponents())
	{
		DetectionCollisionComponents.Add(CollisionComponent);
	}

	if (DetectionComponent)
	{
		DetectionComponent->SetTriggerCollisionComponents(GetAreaDetectionCollisionComponents());
	}

	if (LifeComponent)
	{
		FLxSkillLifeSpec LifeSpec = SkillUnitSpec.LifeSpec;
		LifeSpec.Duration = AreaEffectSpec.Duration;
		LifeComponent->SetLifeSpec(LifeSpec);
	}
}

TArray<UPrimitiveComponent*> ALxAreaSkillUnitActor::GetAreaDetectionCollisionComponents() const
{
	TArray<UPrimitiveComponent*> Result;
	for (UPrimitiveComponent* CollisionComponent : DetectionCollisionComponents)
	{
		if (IsValid(CollisionComponent))
		{
			Result.Add(CollisionComponent);
		}
	}
	return Result;
}

TArray<UPrimitiveComponent*> ALxAreaSkillUnitActor::ResolveAreaDetectionCollisionComponents() const
{
	TArray<UPrimitiveComponent*> Result;
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		const bool bSupportedCollisionShape = PrimitiveComponent
			&& (PrimitiveComponent->IsA<USphereComponent>()
				|| PrimitiveComponent->IsA<UBoxComponent>()
				|| PrimitiveComponent->IsA<UCapsuleComponent>());
		if (bSupportedCollisionShape && PrimitiveComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			Result.Add(PrimitiveComponent);
		}
	}
	return Result;
}

void ALxAreaSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

	if (DetectionComponent)
	{
		DetectionComponent->SetTriggerCollisionComponents(GetAreaDetectionCollisionComponents());
	}

	if (LifeComponent)
	{
		FLxSkillLifeSpec LifeSpec = SkillUnitSpec.LifeSpec;
		LifeSpec.Duration = AreaEffectSpec.Duration;
		LifeComponent->SetLifeSpec(LifeSpec);
	}
}

void ALxAreaSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();

	if (DetectionComponent)
	{
		DetectionComponent->OnDetectionResult.AddUniqueDynamic(this, &ALxAreaSkillUnitActor::HandleAreaDetectionResult);
	}
}

void ALxAreaSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	Super::HandleLifeStateChanged(OldState, NewState);

	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		Destroy();
	}
}

bool ALxAreaSkillUnitActor::ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const
{
	return DetectionResult.EventType != ELxSkillDetectionEventType::OverlapEnd;
}

void ALxAreaSkillUnitActor::ScanCurrentAreaTargets()
{
	if (DetectionCollisionComponents.IsEmpty() || !DetectionComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	for (UPrimitiveComponent* CollisionComponent : DetectionCollisionComponents)
	{
		if (!IsValid(CollisionComponent))
		{
			continue;
		}
		// 刚生成的范围单元可能尚未完成本帧重叠刷新，扫描前主动同步物理重叠状态。
		CollisionComponent->UpdateOverlaps();
		TArray<AActor*> ComponentOverlappingActors;
		CollisionComponent->GetOverlappingActors(ComponentOverlappingActors);
		for (AActor* OverlappingActor : ComponentOverlappingActors)
		{
			OverlappingActors.AddUnique(OverlappingActor);
		}
	}
	DetectionComponent->PublishManualDetectionResult(OverlappingActors);
}

void ALxAreaSkillUnitActor::HandleAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult)
{
	if (DetectionResult.EventType == ELxSkillDetectionEventType::OverlapBegin)
	{
		OnTargetEnterArea.Broadcast(this, DetectionResult);
	}
	else if (DetectionResult.EventType == ELxSkillDetectionEventType::OverlapEnd)
	{
		OnTargetLeaveArea.Broadcast(this, DetectionResult);
	}

	if (TriggerComponent && ShouldProcessAreaDetectionResult(DetectionResult))
	{
		TriggerComponent->HandleDetectionResult(DetectionResult);
	}
}
