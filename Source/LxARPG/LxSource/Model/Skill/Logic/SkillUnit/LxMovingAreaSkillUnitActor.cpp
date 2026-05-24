#include "LxMovingAreaSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"

ALxMovingAreaSkillUnitActor::ALxMovingAreaSkillUnitActor()
{
	MovementComponent = CreateDefaultSubobject<ULxSkillMovementComponent>(TEXT("MovementComponent"));
}

void ALxMovingAreaSkillUnitActor::InitializeSkillUnitDefaultParameters_Implementation()
{
	Super::InitializeSkillUnitDefaultParameters_Implementation();

	if (MovementComponent)
	{
		MovementComponent->SetMovementTargetComponent(GetRootComponent());
	}
}
