#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillPropagationSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxSkillPropagationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillPropagationEvaluated, const FLxSkillPropagationResult&, PropagationResult);

/** 技能传播能力组件，只负责传播次数、剩余计数和传播决策，不直接创建新技能单元。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能传播能力组件")
class LXARPG_API ULxSkillPropagationComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 设置传播参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|传播", DisplayName="设置传播参数")
	void SetPropagationSpec(const FLxSkillPropagationSpec& InPropagationSpec);

	/** 重置传播计数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|传播", DisplayName="重置传播计数")
	void ResetPropagationCounts();

	/** 根据触发结果评估下一步传播能力。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|传播", DisplayName="评估传播能力")
	FLxSkillPropagationResult EvaluatePropagation(const FLxSkillTriggerResult& TriggerResult);

	/** 获取剩余穿透次数。 */
	UFUNCTION(BlueprintPure, Category="技能单元|传播", DisplayName="获取剩余穿透次数")
	int32 GetRemainingPierceCount() const { return RemainingPierceCount; }

	/** 获取剩余弹跳次数。 */
	UFUNCTION(BlueprintPure, Category="技能单元|传播", DisplayName="获取剩余弹跳次数")
	int32 GetRemainingBounceCount() const { return RemainingBounceCount; }

	/** 获取剩余连锁次数。 */
	UFUNCTION(BlueprintPure, Category="技能单元|传播", DisplayName="获取剩余连锁次数")
	int32 GetRemainingChainCount() const { return RemainingChainCount; }

	/** 传播评估事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|传播", DisplayName="传播评估事件")
	FOnLxSkillPropagationEvaluated OnPropagationEvaluated;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="传播参数", meta=(AllowPrivateAccess="true"))
	FLxSkillPropagationSpec PropagationSpec;

	int32 RemainingPierceCount = 0;
	int32 RemainingBounceCount = 0;
	int32 RemainingChainCount = 0;
};
