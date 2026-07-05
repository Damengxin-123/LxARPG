#include "LxDurationAreaSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"

void ALxDurationAreaSkillUnitActor::InitializeDurationAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec,
	const FLxSkillDurationAreaEffectSpec& InDurationAreaEffectSpec)
{
	InitializeAreaEffect(InAreaEffectSpec);
	DurationAreaEffectSpec = InDurationAreaEffectSpec;
	DurationAreaEffectSpec.DetectionPeriod = FMath::Max(DurationAreaEffectSpec.DetectionPeriod, 0.1f);

	if (LifeComponent)
	{
		LifeComponent->SetLifeTickInterval(DurationAreaEffectSpec.DetectionPeriod);
	}
}

void ALxDurationAreaSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	Super::ApplySkillUnitSpecToComponents();

	if (LifeComponent)
	{
		LifeComponent->SetLifeTickInterval(DurationAreaEffectSpec.DetectionPeriod);
	}
}

void ALxDurationAreaSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();

	if (LifeComponent)
	{
		LifeComponent->OnLifeTick.AddUniqueDynamic(this, &ALxDurationAreaSkillUnitActor::HandleAreaDetectionPeriod);
	}
}

bool ALxDurationAreaSkillUnitActor::ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const
{
	return DetectionResult.EventType == ELxSkillDetectionEventType::ManualScan;
}

void ALxDurationAreaSkillUnitActor::HandleAreaDetectionPeriod(float RemainingTime)
{
	ScanCurrentAreaTargets();
}
