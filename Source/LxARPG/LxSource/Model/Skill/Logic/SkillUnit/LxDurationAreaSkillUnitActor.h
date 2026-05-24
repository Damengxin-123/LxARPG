#pragma once

#include "CoreMinimal.h"
#include "LxAreaSkillUnitActor.h"
#include "LxDurationAreaSkillUnitActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxDurationAreaPeriodEvent, float, RemainingTime);

/** 持续范围技能单元类型，在范围单元基础上转发生命周期周期事件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="持续范围技能单元")
class LXARPG_API ALxDurationAreaSkillUnitActor : public ALxAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="技能单元|持续范围", DisplayName="周期触发事件")
	FOnLxDurationAreaPeriodEvent OnAreaPeriodTriggered;

protected:
	virtual void BindSkillUnitComponentEvents() override;

	UFUNCTION()
	void HandleAreaLifeTick(float RemainingTime);
};
