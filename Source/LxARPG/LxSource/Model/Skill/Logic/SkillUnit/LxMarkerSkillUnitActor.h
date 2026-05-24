#pragma once

#include "CoreMinimal.h"
#include "LxSpawnEntitySkillUnitActor.h"
#include "LxMarkerSkillUnitActor.generated.h"

/** 标记技能单元类型，用于放置标记、落点或延迟触发点。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="标记技能单元")
class LXARPG_API ALxMarkerSkillUnitActor : public ALxSpawnEntitySkillUnitActor
{
	GENERATED_BODY()
};
