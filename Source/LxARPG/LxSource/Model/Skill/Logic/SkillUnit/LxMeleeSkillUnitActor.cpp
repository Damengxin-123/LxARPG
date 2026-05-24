#include "LxMeleeSkillUnitActor.h"

void ALxMeleeSkillUnitActor::InitializeMeleeParameters(const FLxSkillMeleeSpec& InMeleeSpec)
{
	MeleeSpec = InMeleeSpec;
}

void ALxMeleeSkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	Super::HandleSkillTriggered(TriggerResult);
	OnMeleeHitTarget.Broadcast(TriggerResult);
}
