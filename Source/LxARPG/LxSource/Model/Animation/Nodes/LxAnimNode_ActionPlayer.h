#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "LxAnimNode_ActionFlow.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxAnimNode_ActionPlayer.generated.h"

/** 根据独立动作通道匹配资源，自动接收速率并输出标准局部空间姿势。 */
USTRUCT(BlueprintInternalUseOnly, meta=(DisplayName="角色动作播放器"))
struct LXARPG_API FLxAnimNode_ActionPlayer : public FLxAnimNode_ActionSource
{
	GENERATED_BODY()

	/** 节点所属通道由基础、近战、远程或防御节点的构造函数确定。 */
	UPROPERTY()
	bool bAttackChannel = false;

	/** 基础节点允许选择类型；攻击节点的类型由具体节点固定。 */
	UPROPERTY(EditAnywhere, Category="动作匹配", meta=(DisplayName="运动类型", NeverAsPin, EditCondition="!bAttackChannel", EditConditionHides))
	ELxCharacterMotionType MotionType = ELxCharacterMotionType::Idle;

	/** 留空匹配该攻击类型的全部技能；填写后要求技能ID完全一致。 */
	UPROPERTY(EditAnywhere, Category="动作匹配", meta=(DisplayName="技能ID", Categories="物品.技能", NeverAsPin, EditCondition="bAttackChannel", EditConditionHides))
	FGameplayTag SkillId;

	/** 支持动画序列、动画合成、混合空间和蒙太奇，编译时检查资源及骨架。 */
	UPROPERTY(EditAnywhere, Category="动作动画", meta=(DisplayName="动画资源", NeverAsPin, AllowedClasses="/Script/Engine.AnimSequence,/Script/Engine.AnimComposite,/Script/Engine.BlendSpace,/Script/Engine.AnimMontage"))
	TObjectPtr<UAnimationAsset> Animation;

	/** 混合空间采样坐标；可连接速度、方向等变量，其他资源忽略此值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="动作动画", meta=(DisplayName="混合空间坐标", PinHiddenByDefault))
	FVector BlendPosition = FVector::ZeroVector;

	/** 供动作骨骼分层混合读取；关闭时有效技能动作覆盖全身。 */
	UPROPERTY(EditAnywhere, Category="动作动画", meta=(DisplayName="是否使用骨骼混合", NeverAsPin))
	bool bUseBoneBlend = true;

	/** 是否允许该播放器产生动画通知；不会改变其他节点或原始动画资产。 */
	UPROPERTY(EditAnywhere, Category="动作动画", meta=(DisplayName="是否接收动画通知", NeverAsPin))
	bool bReceiveAnimationNotifies = true;

	/** 返回缓存和混合所需的当前匹配、优先级及请求标识。 */
	virtual FLxActionPoseState GetActionState() const override;

	/** 初始化内部原生播放器和蒙太奇插槽。 */
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	/** 缓存原生播放器需要的骨骼。 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	/** 在游戏线程获取动作快照并处理蒙太奇生命周期。 */
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	/** 声明该节点需要游戏线程快照。 */
	virtual bool HasPreUpdate() const override { return true; }
	/** 按匹配结果推进内部播放器，重入或重复技能请求时重新开始。 */
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	/** 输出匹配动画；不匹配时保留上一姿势供外部混合过渡，首次无匹配时输出参考姿势。 */
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	/** 显示当前匹配结果及自动播放速率。 */
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	/** 获取测试与调试使用的匹配快照。 */
	bool IsMatched() const { return bMatched; }
	/** 获取自动传入的播放速率，不作为节点配置暴露。 */
	float GetAutomaticPlayRate() const { return AutomaticPlayRate; }
	/** 原生资产推进后按实际进度生成一次结束事件，不使用技能配置时长。 */
	bool CollectPlaybackEnd(UAnimInstance* Instance, FLxCharacterAnimationEvent& OutEvent);

private:
	/** 已上报自然播放结束的释放编号，缓存保持末帧时不重复上报。 */
	FGuid CompletedCastId;
	/** 上次匹配信号时捕获的通知来源，缓存继续播放时仍保持原值。 */
	FLxCharacterAnimationEvent NotifySource;
	/** 执行一次原生播放器更新，通知关闭时在过滤作用域内调用。 */
	void UpdatePlayer(const FAnimationUpdateContext& Context);
	/** 禁用蒙太奇通知时的实例私有副本，避免分支点在队列过滤前执行。 */
	UPROPERTY(Transient, meta=(DisplayName="静默蒙太奇副本"))
	TObjectPtr<UAnimMontage> SilentMontage;
	/** 前一更新是否由缓存要求继续播放。 */
	bool bCachePlaybackLastFrame = false;
	/** 原生序列播放器保留通知、曲线及根运动提取能力。 */
	UPROPERTY()
	FAnimNode_SequencePlayer_Standalone SequencePlayer;
	/** 原生混合空间播放器保留样本插值能力。 */
	UPROPERTY()
	FAnimNode_BlendSpacePlayer_Standalone BlendPlayer;
	/** 蒙太奇使用引擎插槽求值，保留段落、通知与混合语义。 */
	UPROPERTY()
	FAnimNode_Slot MontageSlot;
	/** 游戏线程复制的类型及技能匹配结果。 */
	bool bMatched = false;
	/** 上次图更新是否匹配，用于重入复位。 */
	bool bWasMatched = false;
	/** 上一帧图是否遍历本节点，防止未连接分支启动蒙太奇。 */
	bool bRelevantLastFrame = false;
	/** 已有有效播放器姿势，失配后可保留末帧供外部渐变。 */
	bool bHasPlayed = false;
	/** 从现有动画处理链路复制的播放速率。 */
	float AutomaticPlayRate = 1.0f;
	/** 从通道信号复制的循环标记。 */
	bool bLoop = true;
	/** 当前通道的播放请求编号，支持同技能连续释放。 */
	int32 RequestId = 0;
	/** 上一次已处理的播放请求编号。 */
	int32 LastRequestId = INDEX_NONE;
	/** 当前节点启动的蒙太奇实例编号，停止时不能影响其他节点启动的实例。 */
	int32 OwnedMontageInstanceId = INDEX_NONE;
	/** 蒙太奇已经处理的请求编号。 */
	int32 MontageRequestId = INDEX_NONE;
};
