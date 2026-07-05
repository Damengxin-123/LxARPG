#include "LxPeriodicAuraEffectSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"

void ALxPeriodicAuraEffectSkillUnitActor::InitializePeriodicAuraEffectParameters(
	const FLxSkillPeriodicAuraEffectSpec& InPeriodicSpec)
{
	PeriodicSpec = InPeriodicSpec;
	PeriodicSpec.TriggerInterval = FMath::Max(PeriodicSpec.TriggerInterval, 0.1f);
	ApplySkillUnitSpecToComponents();
}

void ALxPeriodicAuraEffectSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();
	if (LifeComponent)
	{
		LifeComponent->SetLifeTickInterval(PeriodicSpec.TriggerInterval);
	}
}

void ALxPeriodicAuraEffectSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();
	if (LifeComponent)
	{
		LifeComponent->OnLifeTick.AddUniqueDynamic(this,
			&ALxPeriodicAuraEffectSkillUnitActor::HandleAuraEffectPeriod);
	}
}

bool ALxPeriodicAuraEffectSkillUnitActor::CanActivateAuraEffect() const
{
	return Super::CanActivateAuraEffect() && PeriodicSpec.TriggerInterval > 0.0f;
}

void ALxPeriodicAuraEffectSkillUnitActor::HandleAuraEffectActivated()
{
	if (PeriodicSpec.bTriggerImmediately)
	{
		ScanCurrentAuraTargets();
	}
}

void ALxPeriodicAuraEffectSkillUnitActor::HandleAuraDetectionResult(
	const FLxSkillDetectionResult& DetectionResult)
{
	if (!IsSkillUnitActive() || DetectionResult.EventType != ELxSkillDetectionEventType::ManualScan
		|| !TriggerComponent)
	{
		return;
	}

	TriggerComponent->HandleDetectionResult(DetectionResult);
}

void ALxPeriodicAuraEffectSkillUnitActor::HandleAuraEffectPeriod(float RemainingTime)
{
	ScanCurrentAuraTargets();
}
