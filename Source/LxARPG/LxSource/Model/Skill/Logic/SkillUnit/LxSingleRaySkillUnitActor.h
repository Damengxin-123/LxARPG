#pragma once

#include "CoreMinimal.h"
#include "LxRaySkillUnitActor.h"
#include "LxSingleRaySkillUnitActor.generated.h"

/** 单次射线效果单元，激活后只检测一次，并在保留短暂表现后销毁。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="单次射线效果单元")
class LXARPG_API ALxSingleRaySkillUnitActor : public ALxRaySkillUnitActor
{
	GENERATED_BODY()

public:
	virtual void ActivateSkillUnit_Implementation() override;

	/** 初始化单次射线的表现保留时间。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|射线|单次", DisplayName="初始化单次射线参数")
	void InitializeSingleRayParameters(const FLxSingleRayEffectSpec& InSingleRaySpec);

protected:
	/** 当前单次射线的批量创建和表现参数。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|射线|单次", DisplayName="单次射线参数")
	FLxSingleRayEffectSpec SingleRaySpec;
};
