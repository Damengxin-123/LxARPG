#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxCombatCleanupTestReceiver.generated.h"

/** 编辑器自动化测试使用的临时监听器，仅记录公开技能事件。 */
UCLASS(Transient, NotBlueprintable, DisplayName="战斗清理测试监听器")
class ULxCombatCleanupTestReceiver : public UObject
{
	GENERATED_BODY()

public:
	/** 记录检测事件，供测试检查筛选后的目标与场景命中。 */
	UFUNCTION(Category="自动化测试|战斗", DisplayName="记录技能检测结果")
	void ReceiveDetectionResult(const FLxSkillDetectionResult& Result)
	{
		LastDetectionResult = Result;
		++DetectionCount;
	}

	/** 记录触发事件，供测试检查命中次数与间隔限制。 */
	UFUNCTION(Category="自动化测试|战斗", DisplayName="记录技能触发结果")
	void ReceiveTriggerResult(const FLxSkillTriggerResult& Result)
	{
		LastTriggerResult = Result;
		++TriggerCount;
	}

	/** 最近一次公开检测事件的完整结果。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="自动化测试|战斗", DisplayName="最近检测结果")
	FLxSkillDetectionResult LastDetectionResult;

	/** 最近一次公开触发事件的完整结果。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="自动化测试|战斗", DisplayName="最近触发结果")
	FLxSkillTriggerResult LastTriggerResult;

	/** 已收到的检测事件数量。 */
	int32 DetectionCount = 0;

	/** 已收到的触发事件数量。 */
	int32 TriggerCount = 0;
};
