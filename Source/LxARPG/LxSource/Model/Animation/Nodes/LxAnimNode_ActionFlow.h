#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "LxAnimNode_ActionFlow.generated.h"

/** 随姿势链路查询的动作描述，不把动画资源误当作已经求值的姿势。 */
struct LXARPG_API FLxActionPoseState
{
	/** 当前动作是否仍与角色信号匹配。 */
	bool bActive = false;
	/** 当前动作是否使用分层骨骼混合，否则覆盖全身。 */
	bool bUseBoneBlend = true;
	/** 专用技能比通用动作优先，相同优先级按输入顺序选择。 */
	int32 Priority = 0;
	/** 动作及本次播放请求的标识，用于检测切换。 */
	uint32 Key = 0;
};

/** 提供标准姿势输出及动作描述的节点基类。 */
USTRUCT(BlueprintInternalUseOnly, meta=(DisplayName="动作姿势源"))
struct LXARPG_API FLxAnimNode_ActionSource : public FAnimNode_Base
{
	GENERATED_BODY()
	/** 查询当前选择，不推进动画时间，也不发送通知。 */
	virtual FLxActionPoseState GetActionState() const { return {}; }
	/** 设置本次缓存更新的保持与重入要求。 */
	void SetCachePlayback(bool bHold, bool bRestart) { bContinueFromCache = bHold; bRestartFromCache = bRestart; }
	/** 安全解析直接连接的动作源；不匹配的原生节点返回空。 */
	static FLxAnimNode_ActionSource* ResolveSource(FPoseLink& Link, const FAnimationBaseContext& Context);
protected:
	/** 缓存要求失配动作继续播放，单次更新后清除。 */
	bool bContinueFromCache = false;
	/** 缓存切回该动作时从头播放，单次更新后清除。 */
	bool bRestartFromCache = false;
};

/** 从多个动作中选择并保持一个播放器，持续输出其姿势。 */
USTRUCT(BlueprintInternalUseOnly, meta=(DisplayName="动画动作缓存"))
struct LXARPG_API FLxAnimNode_ActionCache : public FLxAnimNode_ActionSource
{
	GENERATED_BODY()
	/** 按顺序连接候选动作；专用技能优先于通用技能。 */
	UPROPERTY(EditAnywhere, Category="动作缓存", meta=(DisplayName="候选动作"))
	TArray<FPoseLink> Actions;
	/** 没有新匹配时保持原动作；循环继续播放，非循环停留在末帧。 */
	UPROPERTY(EditAnywhere, Category="动作缓存", meta=(DisplayName="无匹配时保持动作", NeverAsPin))
	bool bHoldLastAction = true;
	/** 初始化所有候选动作及运行时引用。 */
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	/** 刷新全部候选动作的骨骼缓存。 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	/** 仅更新被选中的动作，未选中的动作不会发出通知。 */
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	/** 求值选中动作；首次没有可用动作时输出参考姿势。 */
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	/** 查询本帧选择，供下游混合节点使用。 */
	virtual FLxActionPoseState GetActionState() const override;
	/** 输出当前缓存索引。 */
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	/** 获取当前真正更新的候选索引，供调试和测试。 */
	int32 GetActiveIndex() const { return ActiveIndex; }
private:
	/** 选择本帧优先级最高的匹配动作。 */
	int32 SelectAction() const;
	/** 初始化时解析的实例内节点引用，不跨动画实例共享。 */
	TArray<FLxAnimNode_ActionSource*> Sources;
	/** 上次选择的动作索引。 */
	int32 ActiveIndex = INDEX_NONE;
};

/** 接收基础和技能动作缓存，按动作选项选择分层混合或全身覆盖。 */
USTRUCT(BlueprintInternalUseOnly, meta=(DisplayName="动作骨骼分层混合"))
struct LXARPG_API FLxAnimNode_ActionLayer : public FLxAnimNode_ActionSource
{
	GENERATED_BODY()
	/** 基础移动动作缓存的姿势。 */
	UPROPERTY(EditAnywhere, Category="动作混合", meta=(DisplayName="基础动作"))
	FPoseLink BasePose;
	/** 攻击或防御动作缓存的姿势；无有效技能时自动使用基础动作。 */
	UPROPERTY(EditAnywhere, Category="动作混合", meta=(DisplayName="技能动作"))
	FPoseLink ActionPose;
	/** 分层混合的起始骨骼和分支深度。 */
	UPROPERTY(EditAnywhere, Category="骨骼混合", meta=(DisplayName="骨骼分层设置", NeverAsPin))
	FInputBlendPose BoneFilter;
	/** 在网格空间混合旋转，适合上半身攻击。 */
	UPROPERTY(EditAnywhere, Category="骨骼混合", meta=(DisplayName="网格空间旋转混合", NeverAsPin))
	bool bMeshSpaceRotationBlend = true;
	/** 初始化内部原生骨骼混合节点。 */
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	/** 刷新骨骼分支权重。 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	/** 根据当前动作状态更新有效的输入。 */
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	/** 输出基础、分层或全身姿势。 */
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	/** 合并基础与动作标识，使下游发现任一有效动作的切换。 */
	virtual FLxActionPoseState GetActionState() const override;
	/** 显示当前是否为全身覆盖。 */
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
private:
	/** 使用引擎原生实现处理骨骼、曲线、属性及根运动权重。 */
	UPROPERTY(meta=(DisplayName="原生骨骼混合器"))
	FAnimNode_LayeredBoneBlend Layer;
	/** 基础动作源，仅引用当前动画实例。 */
	FLxAnimNode_ActionSource* BaseSource = nullptr;
	/** 技能动作源，仅引用当前动画实例。 */
	FLxAnimNode_ActionSource* ActionSource = nullptr;
	/** 当前更新选择全身动作直通。 */
	bool bFullBody = false;
};

/** 一次切换保留的姿势、曲线与动画属性，全部使用堆内存。 */
struct FLxActionPoseSnapshot
{
	/** 紧凑骨骼顺序下的局部变换。 */
	TArray<FTransform> Bones;
	/** 同步保存的动画曲线。 */
	FBlendedHeapCurve Curve;
	/** 同步保存的动画属性。 */
	UE::Anim::FHeapAttributeContainer Attributes;
	/** 本层已经混合的秒数。 */
	float Age = 0.0f;
	/** 保存求值结果，不保存栈分配器引用。 */
	void Store(const FPoseContext& Pose);
	/** 恢复至当前求值上下文。 */
	void Restore(FPoseContext& Pose) const;
};

/** 对已经完成骨骼混合的姿势执行有界堆栈过渡，不重新播放旧动作。 */
USTRUCT(BlueprintInternalUseOnly, meta=(DisplayName="动作姿势混合堆栈"))
struct LXARPG_API FLxAnimNode_ActionStack : public FAnimNode_Base
{
	GENERATED_BODY()
	/** 已完成动作缓存和骨骼混合的姿势。 */
	UPROPERTY(EditAnywhere, Category="动作过渡", meta=(DisplayName="动作姿势"))
	FPoseLink Source;
	/** 动作切换的平滑过渡秒数，零表示立即切换。 */
	UPROPERTY(EditAnywhere, Category="动作过渡", meta=(DisplayName="过渡时间", ClampMin="0", NeverAsPin))
	float BlendTime = 0.2f;
	/** 连续切换保留的最大姿势层数；超出时折叠为当前输出。 */
	UPROPERTY(EditAnywhere, Category="动作过渡", meta=(DisplayName="最大过渡层数", ClampMin="1", ClampMax="8", NeverAsPin))
	int32 MaxBlendDepth = 4;
	/** 初始化链路并清空旧实例姿势。 */
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	/** 骨骼或 LOD 改变时丢弃不兼容的姿势。 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	/** 更新唯一的当前动作，并检测动作或释放请求变化。 */
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	/** 用平滑权重混合历史姿势与当前姿势。 */
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	/** 显示过渡栈深度。 */
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	/** 获取当前过渡层数，供调试和测试。 */
	int32 GetBlendDepth() const { return Stack.Num(); }
private:
	/** 提供动作标识的上游节点。 */
	FLxAnimNode_ActionSource* ActionSource = nullptr;
	/** 从旧到新排列的过渡层。 */
	TArray<FLxActionPoseSnapshot> Stack;
	/** 上一帧未过渡的输入姿势。 */
	FLxActionPoseSnapshot LastInput;
	/** 上一帧最终输出，用于超过层数时连续折叠。 */
	FLxActionPoseSnapshot LastOutput;
	/** 上一更新的动作标识。 */
	uint32 LastKey = 0;
	/** 是否已经收到第一帧动作标识。 */
	bool bHasKey = false;
	/** 当前求值前需要压入新过渡层。 */
	bool bPendingTransition = false;
};
