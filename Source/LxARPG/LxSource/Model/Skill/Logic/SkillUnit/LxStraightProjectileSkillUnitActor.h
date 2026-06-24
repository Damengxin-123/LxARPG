#pragma once

#include "CoreMinimal.h"
#include "LxProjectileSkillUnitActor.h"
#include "LxStraightProjectileSkillUnitActor.generated.h"

/** 直线投射物技能单元，沿创建时的发射方向按投射物基础参数直线飞行。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="直线投射物技能单元")
class LXARPG_API ALxStraightProjectileSkillUnitActor : public ALxProjectileSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 创建直线投射物技能单元。 */
	ALxStraightProjectileSkillUnitActor();
};
