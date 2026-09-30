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

/** 记录真正进入技能蓝图事件的次数，验证等待通知期间不产生技能行为。 */
UCLASS(meta=(DisplayName="技能通知测试"))
class ULxAnimationEventTestSkill : public ULxSkill
{
	GENERATED_BODY()
public:
	/** 实际释放事件次数，蓄力开始不计入。 */
	int32 ExecutionCount = 0;
	/** 仅测试配置释放类型，模拟技能蓝图的类默认值。 */
	void SetTestReleaseType(ELxSkillReleaseType Type) { SkillReleaseType = Type; }
	/** 测试技能事件入口，可捕获原生调用的蓝图实现事件。 */
	virtual void ProcessEvent(UFunction* Function, void* Parameters) override;
};
