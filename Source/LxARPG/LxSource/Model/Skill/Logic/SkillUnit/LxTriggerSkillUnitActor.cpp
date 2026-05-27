#include "LxTriggerSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxTriggerSkillUnitActor::ALxTriggerSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerCollision"));
	TriggerCollisionComponent->SetupAttachment(SceneRoot);
	TriggerCollisionComponent->SetSphereRadius(100.0f);
	TriggerCollisionComponent->SetGenerateOverlapEvents(true);

	DetectionComponent = CreateDefaultSubobject<ULxSkillDetectionComponent>(TEXT("DetectionComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

void ALxTriggerSkillUnitActor::InitializeTriggerUnitParameters(const FLxSkillTriggerUnitSpec& InTriggerUnitSpec)
{
	TriggerUnitSpec = InTriggerUnitSpec;
}

void ALxTriggerSkillUnitActor::InitializeSkillUnitDefaultParameters_Implementation()
{
	Super::InitializeSkillUnitDefaultParameters_Implementation();

	if (DetectionComponent)
	{
		DetectionComponent->SetTriggerCollisionComponent(TriggerCollisionComponent);
	}
}

void ALxTriggerSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

	if (TriggerCollisionComponent && SkillUnitSpec.SpaceSpec.Radius > 0.0f)
	{
		TriggerCollisionComponent->SetSphereRadius(SkillUnitSpec.SpaceSpec.Radius);
	}
}

void ALxTriggerSkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	Super::HandleSkillTriggered(TriggerResult);
	OnTriggerUnitTriggered.Broadcast(this, TriggerResult);
}

void ALxTriggerSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	Super::HandleLifeStateChanged(OldState, NewState);

	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		OnTriggerUnitExpired.Broadcast(this, MakeSkillUnitResult(ELxSkillUnitResultType::Expired, true));
	}
}
