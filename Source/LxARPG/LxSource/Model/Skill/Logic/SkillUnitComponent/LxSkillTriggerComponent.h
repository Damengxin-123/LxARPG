#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillHitLimitSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillTargetFilterSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillTriggerSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxSkillTriggerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillTriggered, const FLxSkillTriggerResult&, TriggerResult);

/** 技能触发能力组件，只负责将检测结果、周期时钟或手动请求转换为最终触发结果。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能触发能力组件")
class LXARPG_API ULxSkillTriggerComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 设置触发节奏参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="设置触发参数")
	void SetTriggerSpec(const FLxSkillTriggerSpec& InTriggerSpec);

	/** 设置目标筛选参数，主要用于深度判断标签、存活状态等触发条件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="设置目标筛选参数")
	void SetTargetFilterSpec(const FLxSkillTargetFilterSpec& InTargetFilterSpec);

	/** 设置命中限制参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="设置命中限制参数")
	void SetHitLimitSpec(const FLxSkillHitLimitSpec& InHitLimitSpec);

	/** 开始触发判断。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="开始触发判断")
	void StartTrigger();

	/** 停止触发判断。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="停止触发判断")
	void StopTrigger();

	/** 接收检测组件广播的候选目标，判断是否触发。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="处理检测结果")
	void HandleDetectionResult(const FLxSkillDetectionResult& DetectionResult);

	/** 接收生命周期周期事件，通常用于持续区域、光环、附着等周期触发。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="处理生命周期周期")
	void HandleLifeTick(float RemainingTime);

	/** 手动请求触发，适合输入、引爆或脚本主动触发。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="手动请求触发")
	void RequestTrigger(const FLxSkillDetectionResult& DetectionResult);

	/** 清理指定目标的触发记录，使光环等持续效果在目标重新进入时可以再次触发。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发", DisplayName="清理目标触发记录")
	void ResetTargetTriggerRecord(AActor* InTarget);

	/** 最终触发事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|触发", DisplayName="触发事件")
	FOnLxSkillTriggered OnTriggered;

private:
	bool CanTriggerTarget(AActor* InTarget) const;
	void BroadcastTriggerResult(const FLxSkillDetectionResult& DetectionResult, const TArray<AActor*>& TriggeredTargets);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="触发参数", meta=(AllowPrivateAccess="true"))
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="目标筛选参数", meta=(AllowPrivateAccess="true"))
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="命中限制参数", meta=(AllowPrivateAccess="true"))
	FLxSkillHitLimitSpec HitLimitSpec;

	TMap<TWeakObjectPtr<AActor>, int32> TargetHitCounts;
	TMap<TWeakObjectPtr<AActor>, float> TargetLastTriggerTimes;
	int32 TotalTriggerCount = 0;
	bool bTriggering = false;
};
