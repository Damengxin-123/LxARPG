#pragma once

#include "CoreMinimal.h"
#include "LxAreaSkillUnitActor.h"
#include "LxMovingAreaSkillUnitActor.generated.h"

class ULxSkillMovementComponent;

/** 移动范围技能单元类型，在范围单元基础上增加运动能力组件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="移动范围技能单元")
class LXARPG_API ALxMovingAreaSkillUnitActor : public ALxAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxMovingAreaSkillUnitActor();

protected:
	virtual void InitializeSkillUnitDefaultParameters_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="运动能力组件")
	TObjectPtr<ULxSkillMovementComponent> MovementComponent;
};
