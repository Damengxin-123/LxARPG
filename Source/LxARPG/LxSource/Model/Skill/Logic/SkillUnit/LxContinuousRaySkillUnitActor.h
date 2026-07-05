#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillRaySpec.h"
#include "LxRaySkillUnitActor.h"
#include "LxContinuousRaySkillUnitActor.generated.h"

/** 持续射线效果单元，启用后按固定周期重新执行射线检测，停止后可再次启用。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="持续射线效果单元")
class LXARPG_API ALxContinuousRaySkillUnitActor : public ALxRaySkillUnitActor
{
	GENERATED_BODY()

public:
	/** 创建默认隐藏的持续射线单元，只在激活期间显示。 */
	ALxContinuousRaySkillUnitActor();

	/** 初始化持续射线的周期检测参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|射线|持续", DisplayName="初始化持续射线参数")
	void InitializeContinuousRayParameters(const FLxContinuousRayEffectSpec& InContinuousRaySpec);

	virtual void ActivateSkillUnit_Implementation() override;
	virtual void StopSkillUnit_Implementation() override;
	virtual void CancelSkillUnit_Implementation() override;

protected:
	/** 执行一次持续射线周期检测。 */
	UFUNCTION()
	void HandleContinuousRayTick();

	/** 清除持续射线周期计时器。 */
	void ClearContinuousRayTimer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线|持续", DisplayName="持续射线参数")
	FLxContinuousRayEffectSpec ContinuousRaySpec;

	FTimerHandle ContinuousRayTimerHandle;
};
