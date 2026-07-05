#pragma once

#include "CoreMinimal.h"
#include "LxAreaSkillUnitActor.h"
#include "LxDirectHitAreaSkillUnitActor.generated.h"

/** 直接命中型范围效果，激活后只判定一次目标，但表现持续到设定时间结束。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="直接命中型范围效果技能子单元")
class LXARPG_API ALxDirectHitAreaSkillUnitActor : public ALxAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 激活后执行一次完整范围判定；持续时间结束后由生命周期组件销毁自身。 */
	virtual void ActivateSkillUnit_Implementation() override;

protected:
	virtual bool ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const override;

	/** 下一 Tick 执行首次范围扫描，等待新生成碰撞体进入物理场景。 */
	void HandleDeferredInitialScan();
};
