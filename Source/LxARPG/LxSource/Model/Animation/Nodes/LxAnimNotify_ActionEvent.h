#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LxAnimNotify_ActionEvent.generated.h"

/** 在动画时间轴标记角色动作事件，由动画实例统一收集并逐层上报。 */
UCLASS(meta=(DisplayName="角色动作通知"))
class LXARPG_API ULxAnimNotify_ActionEvent : public UAnimNotify
{
	GENERATED_BODY()
public:
	/** 技能释放执行技能实体创建；技能结束完成本次施法动画。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色动画|通知", meta=(DisplayName="通知名称"))
	FName EventName = TEXT("技能释放");

	/** 在动画时间轴显示实际通知名称。 */
	virtual FString GetNotifyName_Implementation() const override { return EventName.ToString(); }
	/** 分支点仅排队，避免在蒙太奇推进中直接创建技能或停止动画。 */
	virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};
