#pragma once

#include "Animation/AnimNodeMessages.h"
#include "Animation/AnimInstanceProxy.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"

struct FLxAnimNode_ActionPlayer;

/** 为序列、合成和混合空间通知保存原始播放身份。 */
class FLxActionEventContext : public UE::Anim::IAnimNotifyEventContextDataInterface
{
	DECLARE_NOTIFY_CONTEXT_INTERFACE(FLxActionEventContext)
public:
	/** 复制动作播放器在游戏线程取得的身份快照。 */
	explicit FLxActionEventContext(const FLxCharacterAnimationEvent& InEvent) : Event(InEvent) {}
	/** 不随动画实例当前信号改变的通知来源。 */
	FLxCharacterAnimationEvent Event;
};

/** 将动作身份写入原生资产播放器的通知上下文。 */
class FLxActionEventScope : public UE::Anim::IGraphMessage
{
	DECLARE_ANIMGRAPH_MESSAGE(FLxActionEventScope)
public:
	/** 保存当前动作的播放身份。 */
	explicit FLxActionEventScope(const FLxCharacterAnimationEvent& InEvent) : Event(InEvent) {}
	/** 每个通知持有独立的身份快照。 */
	virtual TUniquePtr<const UE::Anim::IAnimNotifyEventContextDataInterface> MakeUniqueEventContextData() const override;
private:
	/** 本作用域内播放器的通知来源。 */
	FLxCharacterAnimationEvent Event;
};

/** 标记来自关闭通知的动作播放器的事件，按播放来源过滤而不修改资源。 */
class FLxDisabledActionNotifyContext : public UE::Anim::IAnimNotifyEventContextDataInterface
{
	DECLARE_NOTIFY_CONTEXT_INTERFACE(FLxDisabledActionNotifyContext)
};

/** 把当前节点的通知开关传入原生播放器生成的事件上下文。 */
class FLxActionNotifyScope : public UE::Anim::IGraphMessage
{
	DECLARE_ANIMGRAPH_MESSAGE(FLxActionNotifyScope)
public:
	/** 为通知创建关闭标记，仅对本作用域内的播放器生效。 */
	virtual TUniquePtr<const UE::Anim::IAnimNotifyEventContextDataInterface> MakeUniqueEventContextData() const override;
};

/** 在游戏线程分发通知前移除被动作节点禁用的事件。 */
struct FLxActionAnimInstanceProxy : public FAnimInstanceProxy
{
	/** 每帧清空实际被动画图更新的播放器记录。 */
	virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override;
	/** 记录最终动作链路选择的播放器，用实际播放进度判断结束。 */
	void RegisterActionPlayer(FLxAnimNode_ActionPlayer* Player) { UpdatedPlayers.AddUnique(Player); }
	/** 使用原生代理初始化全部动画运行状态。 */
	explicit FLxActionAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
	/** 过滤单次与状态通知，已有状态的结束清理由引擎继续执行。 */
	virtual void PostUpdate(UAnimInstance* Instance) const override;
private:
	/** 仅在所属动画图更新任务内写入，主线程完成任务后读取。 */
	TArray<FLxAnimNode_ActionPlayer*> UpdatedPlayers;
};
