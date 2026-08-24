#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxCharacterAnimationProcessComponent.generated.h"

class ULxAnimInstanceBase;
class ULxCharacterBehaviorControlComponent;

/**
 * 角色动画处理组件。
 * 接收运动分析组件输出的运动信号，并转换为动画实例可执行的动画播放信号。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色动画处理组件")
class LXARPG_API ULxCharacterAnimationProcessComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterAnimationProcessComponent();

	/** 初始化动画处理组件并绑定角色行为控制组件事件。 */
	virtual void BaseComponentInitialize() override;

	/** 组件销毁时解除运动信号绑定。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 接收运动分析组件输出的基础运动信号。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|动画处理", DisplayName="接收基础运动信号")
	void ReceiveBaseMotionSignal(const FLxCharacterMotionSignal& InMotionSignal);

	/** 接收运动分析组件输出的动作运动信号。 */
	UFUNCTION(BlueprintCallable, Category="角色动画|动画处理", DisplayName="接收动作运动信号")
	void ReceiveActionMotionSignal(const FLxCharacterMotionSignal& InMotionSignal);

	/** 获取最近一次生成的基础动画播放信号。 */
	UFUNCTION(BlueprintPure, Category="角色动画|动画处理", DisplayName="获取当前基础动画信号")
	const FLxCharacterAnimationSignal& GetCurrentBaseAnimationSignal() const { return CurrentBaseAnimationSignal; }

	/** 获取最近一次生成的动作动画播放信号。 */
	UFUNCTION(BlueprintPure, Category="角色动画|动画处理", DisplayName="获取当前动作动画信号")
	const FLxCharacterAnimationSignal& GetCurrentActionAnimationSignal() const { return CurrentActionAnimationSignal; }

	/** 获取行为控制组件最近同步到动画层的全部即时行为状态。 */
	UFUNCTION(BlueprintPure, Category="角色动画|行为状态", DisplayName="获取当前动画行为状态")
	const FGameplayTagContainer& GetCurrentBehaviorStateTags() const { return CurrentBehaviorStateTags; }

protected:
	/** 将运动信号转换为动画播放信号，蓝图可重写以接入职业、武器、Buff 等配置。 */
	UFUNCTION(BlueprintNativeEvent, Category="角色动画|动画处理", DisplayName="转换为动画信号")
	FLxCharacterAnimationSignal ConvertMotionSignalToAnimationSignal(const FLxCharacterMotionSignal& InMotionSignal) const;
	virtual FLxCharacterAnimationSignal ConvertMotionSignalToAnimationSignal_Implementation(const FLxCharacterMotionSignal& InMotionSignal) const;

private:
	/** 接收统一行为控制组件广播的即时状态变化。 */
	UFUNCTION()
	void ReceiveBehaviorStateChanged(FGameplayTag InBehaviorStateTag, bool bInActive);

	void CacheOwnerComponents();
	void BindBehaviorControlEvents();
	void UnbindBehaviorControlEvents();
	void EnsureAnimationInstanceCached();

	/** 当前角色的行为控制组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterBehaviorControlComponent> BehaviorControlComponent = nullptr;

	/** 当前角色网格使用的动画实例。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxAnimInstanceBase> AnimInstance = nullptr;

	/** 最近一次生成的基础动画播放信号。 */
	UPROPERTY(Transient)
	FLxCharacterAnimationSignal CurrentBaseAnimationSignal;

	/** 最近一次生成的动作动画播放信号。 */
	UPROPERTY(Transient)
	FLxCharacterAnimationSignal CurrentActionAnimationSignal;

	/** 当前动画层缓存的角色即时行为状态。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色动画|行为状态", DisplayName="当前动画行为状态",
		meta=(AllowPrivateAccess="true"))
	FGameplayTagContainer CurrentBehaviorStateTags;

	/** 动画处理组件是否正在初始化，用于阻止运动信号回调造成重入。 */
	bool bAnimationProcessInitializing = false;

	/** 动画处理组件是否已经完成初始化。 */
	bool bAnimationProcessInitialized = false;
};
