#include "LxBeamSkillUnitActor.h"

void ALxBeamSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState)
{
	Super::HandleLifeStateChanged(OldState, NewState);

	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		OnBeamDurationFinished.Broadcast();
	}
}
