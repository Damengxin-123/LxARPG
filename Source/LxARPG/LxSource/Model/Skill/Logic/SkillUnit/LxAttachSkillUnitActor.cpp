#include "LxAttachSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

ALxAttachSkillUnitActor::ALxAttachSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
}

void ALxAttachSkillUnitActor::InitializeAttachParameters(const FLxSkillAttachSpec& InAttachSpec)
{
	AttachSpec = InAttachSpec;
}

void ALxAttachSkillUnitActor::SetAttachTarget(AActor* InAttachTarget)
{
	AttachTarget = InAttachTarget;
	if (!AttachTarget)
	{
		OnAttachTargetInvalid.Broadcast(this, nullptr);
	}
}

void ALxAttachSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();

	if (LifeComponent)
	{
		LifeComponent->OnLifeTick.AddUniqueDynamic(this, &ALxAttachSkillUnitActor::HandleAttachLifeTick);
	}
}

void ALxAttachSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	Super::HandleLifeStateChanged(OldState, NewState);

	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		OnAttachExpired.Broadcast(this, MakeSkillUnitResult(ELxSkillUnitResultType::Expired, true));
	}
}

void ALxAttachSkillUnitActor::HandleAttachLifeTick(float RemainingTime)
{
	if (!AttachTarget || AttachTarget->IsPendingKillPending())
	{
		OnAttachTargetInvalid.Broadcast(this, AttachTarget);
		return;
	}

	OnAttachPeriodTriggered.Broadcast(this, RemainingTime);
}
