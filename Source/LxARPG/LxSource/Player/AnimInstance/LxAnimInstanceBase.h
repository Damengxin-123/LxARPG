// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationAssetConfig.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxARPG/LxSource/Player/Characters/LxCharacterStateEnum.h"
#include "LxAnimInstanceBase.generated.h"

class ALxBaseCharacter;
class UAnimationAsset;
/**
 * 
 */
UCLASS(Blueprintable, DisplayName="角色动画类型基类")
class LXARPG_API ULxAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()
public:
	/** 收集原生排队通知；只接受事件携带的来源，不能借用当前技能编号。 */
	void CollectAnimationNotify(const FAnimNotifyEventReference& Event);
	/** 收集角色动作通知的蒙太奇分支点。 */
	void CollectActionBranchingPoint(FName NotifyName, int32 MontageInstanceId);
	/** 注册蒙太奇实例与动作播放身份的对应关系。 */
	void RegisterActionMontageSource(int32 InstanceId, const FLxCharacterAnimationEvent& Source);
	/** 收集播放器按实际播放进度生成的结束事件，保持与通知相同的回传路径。 */
	void CollectPlaybackEvent(const FLxCharacterAnimationEvent& Event) { QueueAnimationEvent(Event); }
	/** 姿势刷新后在游戏线程统一向动画处理组件发送通知。 */
	virtual void NativePostEvaluateAnimation() override;
	/** 动画蓝图可统一观察已收集的通知，无需为每个技能分别连线。 */
	UFUNCTION(BlueprintImplementableEvent, Category="角色动画|通知", meta=(DisplayName="收到角色动画通知"))
	void ReceiveAnimationEvent(const FLxCharacterAnimationEvent& Event);
	/** 向上层组件发送通知的原生事件。 */
	FOnLxCharacterAnimationEvent OnAnimationEvent;
	/** 创建支持动作节点通知开关的动画代理。 */
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	/** 禁用指定动作蒙太奇实例的排队通知，不影响同资源的其他实例。 */
	void DisableActionMontageNotifies(int32 InstanceId) { DisabledActionMontageInstances.Add(InstanceId); }
	/** 判断蒙太奇通知是否属于已关闭接收的播放实例。 */
	bool IsActionMontageNotifyDisabled(int32 InstanceId) const { return DisabledActionMontageInstances.Contains(InstanceId); }
	/** 在通知队列过滤完毕后移除已经销毁的蒙太奇实例记录。 */
	void PruneActionMontageNotifyFilters();
private:
	/** 主动画实例统一存放本帧通知，避免更新动画时重入技能逻辑。 */
	TArray<FLxCharacterAnimationEvent> PendingAnimationEvents;
	/** 蒙太奇排队通知和分支点共用的播放身份映射。 */
	TMap<int32, FLxCharacterAnimationEvent> MontageSources;
	/** 将链接动画实例的通知汇入主动画实例。 */
	void QueueAnimationEvent(const FLxCharacterAnimationEvent& Event);
	/** 按播放实例记录通知开关，防止其他节点使用同动画时被误过滤。 */
	TSet<int32> DisabledActionMontageInstances;
public:
	/**
	 * @brief 初始化动画实例。
	 *
	 * 会在动画蓝图实例创建后缓存所属角色并建立状态动画映射。
	 */
	virtual void NativeInitializeAnimation() override;

	/**
	 * @brief 每帧更新动画实例状态。
	 *
	 * @param DeltaSeconds 当前帧与上一帧之间的时间差。
	 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="角色状态", DisplayName="角色状态")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;

	/** 各个角色动画类型对应的动画资产配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色动画|动画配置", DisplayName="动画资产配置")
	TArray<FLxCharacterAnimationAssetConfig> AnimationAssetConfigs;

	/** 找不到基础动画类型对应配置时使用的默认动画资产。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色动画|动画配置", DisplayName="默认动画资产")
	TObjectPtr<UAnimationAsset> DefaultAnimationAsset;

	/** 行为树与动画图共用的动作标识，每帧在游戏线程从行为控制组件同步。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|运动", meta=(DisplayName="当前运动类型"))
	ELxCharacterMotionType CurrentMotionType = ELxCharacterMotionType::Idle;

	/** 基础通道的实时运动类型，攻击开始或结束不会覆盖此值。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|基础动作", meta=(DisplayName="基础动作运动类型"))
	ELxCharacterMotionType BaseMotionType = ELxCharacterMotionType::Idle;

	/** 攻击通道的实时运动类型，无表示当前没有攻击或防御。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|攻击动作", meta=(DisplayName="攻击动作运动类型"))
	ELxCharacterMotionType AttackMotionType = ELxCharacterMotionType::None;

	/** 当前正在释放的技能ID，供动作节点精确匹配。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|攻击动作", meta=(DisplayName="当前攻击技能ID", Categories="物品.技能"))
	FGameplayTag CurrentAttackSkillId;

	/** 检查节点是否匹配；攻击技能ID留空时只检查类型，填写时精确匹配。 */
	UFUNCTION(BlueprintPure, Category="角色动画|动作匹配", meta=(DisplayName="动作节点是否匹配", BlueprintThreadSafe))
	bool MatchesMotion(bool bAttackChannel, ELxCharacterMotionType MotionType, FGameplayTag SkillId) const;

	/** 当前基础动画播放信号，由外部动画处理组件推送。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|基础动画", DisplayName="当前基础动画信号")
	FLxCharacterAnimationSignal CurrentBaseAnimationSignal;

	/** 当前基础动画信号指定的动画类型。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|基础动画", DisplayName="当前基础动画类型")
	ELxCharacterMotionType CurrentBaseAnimationType = ELxCharacterMotionType::Idle;

	/** 当前基础动画信号指定的播放速率。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|基础动画", DisplayName="当前基础动画播放速率")
	float CurrentBaseAnimationPlayRate = 1.0f;

	/** 当前基础动画信号指定的循环播放标记。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|基础动画", DisplayName="当前基础动画是否循环")
	bool bCurrentBaseAnimationLoop = true;

	/** 当前基础动画类型解析出的动画资产，保证优先使用配置并在缺失时使用默认资产。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|基础动画", DisplayName="当前基础动画资产")
	TObjectPtr<UAnimationAsset> CurrentBaseAnimationAsset;

	/** 当前动作动画播放信号，由外部动画处理组件推送。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="当前动作动画信号")
	FLxCharacterAnimationSignal CurrentActionAnimationSignal;

	/** 当前动作动画信号指定的动画类型，无表示当前没有动作动画。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="当前动作动画类型")
	ELxCharacterMotionType CurrentActionAnimationType = ELxCharacterMotionType::None;

	/** 当前动作动画信号指定的播放速率。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="当前动作动画播放速率")
	float CurrentActionAnimationPlayRate = 1.0f;

	/** 当前动作动画信号指定的循环播放标记。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="当前动作动画是否循环")
	bool bCurrentActionAnimationLoop = false;

	/** 当前动作动画类型解析出的动画资产，未配置动作时允许为空。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="当前动作动画资产")
	TObjectPtr<UAnimationAsset> CurrentActionAnimationAsset;

	/** 当前是否存在有效动作动画，动画图表可将其转换为动作层融合权重。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="是否需要混合动作动画")
	bool bShouldBlendActionAnimation = false;

	/** 每次收到动作动画信号时递增，用于通知动画图表重新播放相同的动作资产。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category="角色动画|动作动画", DisplayName="动作动画播放请求编号")
	int32 ActionAnimationPlayRequestId = 0;

	/** 应用外部动画处理组件生成的基础动画播放信号。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|基础动画", DisplayName="应用基础动画信号")
	void ApplyBaseAnimationSignal(const FLxCharacterAnimationSignal& InAnimationSignal);

	/** 应用外部动画处理组件生成的动作动画播放信号。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|动作动画", DisplayName="应用动作动画信号")
	void ApplyActionAnimationSignal(const FLxCharacterAnimationSignal& InAnimationSignal);

	/** 根据基础动画信号更新基础动画资产和播放参数变量。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|基础动画", DisplayName="更新基础动画播放")
	void UpdateBaseAnimationPlayback(const FLxCharacterAnimationSignal& InAnimationSignal);

	/** 根据动作动画信号更新动作动画资产、播放参数和融合标记。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|动作动画", DisplayName="更新动作动画播放")
	void UpdateActionAnimationPlayback(const FLxCharacterAnimationSignal& InAnimationSignal);

	/** 根据动画类型获取配置的动画资产，未找到时返回空。 */
	UFUNCTION(BlueprintPure, Category="角色动画|动画配置", DisplayName="获取动画类型对应资产")
	UAnimationAsset* GetConfiguredAnimationAsset(ELxCharacterMotionType InAnimationType) const;
	
protected:
	// 持有此动画蓝图的角色
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pCharacter;

	/** 动画类型到动画资产的运行时缓存。 */
	UPROPERTY(Transient)
	TMap<ELxCharacterMotionType, TObjectPtr<UAnimationAsset>> AnimationAssetMap;
};
