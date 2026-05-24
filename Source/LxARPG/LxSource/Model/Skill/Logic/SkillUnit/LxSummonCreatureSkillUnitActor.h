#pragma once

#include "CoreMinimal.h"
#include "LxSpawnEntitySkillUnitActor.h"
#include "LxSummonCreatureSkillUnitActor.generated.h"

/** 召唤生物技能单元类型，用于召唤具有自主AI的角色单位。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="召唤生物技能单元")
class LXARPG_API ALxSummonCreatureSkillUnitActor : public ALxSpawnEntitySkillUnitActor
{
	GENERATED_BODY()
};
