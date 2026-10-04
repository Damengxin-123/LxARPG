#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxAnimationActionFlowTestNotify.generated.h"

/** 自动化测试使用的通知计数器，不创建或修改用户资产。 */
UCLASS(meta=(DisplayName="动作通知测试"))
class ULxAnimationActionFlowTestNotify : public UAnimNotify
{
	GENERATED_BODY()
public:
	/** 当前测试收到的普通通知数量。 */
	static int32 Count;
	/** 记录实际经由引擎分发的通知。 */
	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Event) override;
};

/** 记录真正交给流程资产的释放次数，验证等待通知期间不产生技能行为。 */
UCLASS(meta=(DisplayName="技能通知测试"))
class ULxAnimationEventTestSkill : public ULxSkill
{
	GENERATED_BODY()
public:
	/** 实际释放事件次数，蓄力开始不计入。 */
	int32 ExecutionCount = 0;
	/** 配置内存中的流程资产与对应入口，不修改项目资产。 */
	void SetTestReleaseType(ELxSkillReleaseType Type);
	/** 记录实际发送到新流程的释放事件，并执行正常流程。 */
	virtual bool DispatchFlowEvent(ELxSkillFlowEvent Event) override;
};
