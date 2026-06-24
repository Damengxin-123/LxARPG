#include "LxAreaSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxAreaSkillUnitActor::ALxAreaSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AreaCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("AreaCollision"));
	AreaCollisionComponent->SetupAttachment(SceneRoot);
	AreaCollisionComponent->SetSphereRadius(100.0f);
	AreaCollisionComponent->SetGenerateOverlapEvents(true);

	DetectionComponent = CreateDefaultSubobject<ULxSkillDetectionComponent>(TEXT("DetectionComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxAreaSkillUnitActor::InitializeAreaParameters(const FLxSkillAreaSpec& InAreaSpec)
{
	AreaSpec = InAreaSpec;
}

void ALxAreaSkillUnitActor::InitializeSkillUnitDefaultParameters_Implementation()
{
	Super::InitializeSkillUnitDefaultParameters_Implementation();

	if (DetectionComponent)
	{
		DetectionComponent->SetTriggerCollisionComponent(AreaCollisionComponent);
	}
}

void ALxAreaSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

	const float RadiusCm = SkillUnitSpec.SpaceSpec.GetRadiusInUnrealUnits();
	if (AreaCollisionComponent && RadiusCm > 0.0f)
	{
		AreaCollisionComponent->SetSphereRadius(RadiusCm);
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
}
