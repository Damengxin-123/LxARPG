#pragma once

#include "CoreMinimal.h"
#include "LxRaySkillUnitActor.h"
#include "LxBeamSkillUnitActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxBeamFinished);

/** 持续射线技能单元类型，在射线单元基础上增加持续结束语义。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="持续射线技能单元")
class LXARPG_API ALxBeamSkillUnitActor : public ALxRaySkillUnitActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="技能单元|持续射线", DisplayName="射线结束持续")
	FOnLxBeamFinished OnBeamDurationFinished;

protected:
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;
};
