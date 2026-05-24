#pragma once

#include "CoreMinimal.h"
#include "LxSpawnEntitySkillUnitActor.h"
#include "LxBarrierSkillUnitActor.generated.h"

/** 召唤屏障技能单元类型，用于召唤具有阻挡或保护语义的实体。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="召唤屏障技能单元")
class LXARPG_API ALxBarrierSkillUnitActor : public ALxSpawnEntitySkillUnitActor
{
	GENERATED_BODY()
};
