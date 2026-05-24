#pragma once

#include "CoreMinimal.h"
#include "LxAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMeleeSpec.h"
#include "LxMeleeSkillUnitActor.generated.h"

/** 近战技能单元类型，本质上使用范围检测，但事件语义按近战命中转发。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="近战技能单元")
class LXARPG_API ALxMeleeSkillUnitActor : public ALxAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 初始化近战效果参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|近战", DisplayName="初始化近战效果参数")
	void InitializeMeleeParameters(const FLxSkillMeleeSpec& InMeleeSpec);

	UPROPERTY(BlueprintAssignable, Category="技能单元|近战", DisplayName="近战命中目标")
	FOnLxSkillUnitTriggerEvent OnMeleeHitTarget;

protected:
	virtual void HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|近战", DisplayName="近战参数")
	FLxSkillMeleeSpec MeleeSpec;
};
