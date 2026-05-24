#include "LxDurationAreaSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"

void ALxDurationAreaSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();

	if (LifeComponent)
	{
		LifeComponent->OnLifeTick.AddUniqueDynamic(this, &ALxDurationAreaSkillUnitActor::HandleAreaLifeTick);
	}
}

void ALxDurationAreaSkillUnitActor::HandleAreaLifeTick(float RemainingTime)
{
	OnAreaPeriodTriggered.Broadcast(RemainingTime);
}
