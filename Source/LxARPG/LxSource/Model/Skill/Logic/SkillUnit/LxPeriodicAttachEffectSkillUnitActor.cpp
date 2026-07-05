#include "LxPeriodicAttachEffectSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"

void ALxPeriodicAttachEffectSkillUnitActor::InitializePeriodicAttachEffectParameters(
	const FLxSkillPeriodicAttachEffectSpec& InPeriodicSpec)
{
	PeriodicSpec = InPeriodicSpec;
	PeriodicSpec.TriggerInterval = FMath::Max(PeriodicSpec.TriggerInterval, 0.1f);
	ApplySkillUnitSpecToComponents();
}

void ALxPeriodicAttachEffectSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();
	if (LifeComponent)
	{
		LifeComponent->SetLifeTickInterval(PeriodicSpec.TriggerInterval);
	}
}

void ALxPeriodicAttachEffectSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();
	if (LifeComponent)
	{
		LifeComponent->OnLifeTick.AddUniqueDynamic(this, &ALxPeriodicAttachEffectSkillUnitActor::HandleAttachEffectPeriod);
	}
}

bool ALxPeriodicAttachEffectSkillUnitActor::CanActivateAttachEffect() const
{
	return PeriodicSpec.TriggerInterval > 0.0f;
}

void ALxPeriodicAttachEffectSkillUnitActor::HandleAttachEffectActivated()
{
	if (PeriodicSpec.bTriggerImmediately)
	{
		TriggerAttachTargetHit();
	}
}

void ALxPeriodicAttachEffectSkillUnitActor::HandleAttachEffectPeriod(float RemainingTime)
{
	if (!IsValid(AttachTarget))
	{
		EndAttachEffect(ELxAttachEffectEndReason::TargetInvalid, ELxSkillUnitResultType::Cancelled, false);
		return;
	}
	TriggerAttachTargetHit();
}
