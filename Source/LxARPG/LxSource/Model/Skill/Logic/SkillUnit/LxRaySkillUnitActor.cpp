#include "LxRaySkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillPropagationComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxRaySkillUnitActor::ALxRaySkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DetectionComponent = CreateDefaultSubobject<ULxSkillDetectionComponent>(TEXT("DetectionComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	PropagationComponent = CreateDefaultSubobject<ULxSkillPropagationComponent>(TEXT("PropagationComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxRaySkillUnitActor::InitializeRayParameters(const FLxSkillRaySpec& InRaySpec)
{
	RaySpec = InRaySpec;
}

void ALxRaySkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	Super::HandleSkillTriggered(TriggerResult);
	OnRayHitTarget.Broadcast(TriggerResult);
}
