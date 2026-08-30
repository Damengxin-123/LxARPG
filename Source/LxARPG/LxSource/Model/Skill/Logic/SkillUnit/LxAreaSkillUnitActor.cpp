#include "LxAreaSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxAreaSkillUnitActor::ALxAreaSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxAreaSkillUnitActor::InitializeAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec)
{
	AreaEffectSpec = InAreaEffectSpec;
	RefreshSkillUnitOverlapEventSources();

	if (LifeComponent)
	{
		FLxSkillLifeSpec LifeSpec = SkillUnitSpec.LifeSpec;
		LifeSpec.Duration = AreaEffectSpec.Duration;
		LifeComponent->SetLifeSpec(LifeSpec);
	}
}

TArray<UPrimitiveComponent*> ALxAreaSkillUnitActor::GetAreaDetectionCollisionComponents() const
{
	return GetSkillUnitOverlapEventSources();
}

void ALxAreaSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

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
	const TArray<UPrimitiveComponent*> CollisionComponents = GetAreaDetectionCollisionComponents();
	if (CollisionComponents.IsEmpty() || !DetectionComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	for (UPrimitiveComponent* CollisionComponent : CollisionComponents)
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
