#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxCharacterAnimationProcessComponent.generated.h"

class ULxAnimInstanceBase;
class ULxCharacterAnimationMotionAnalysisComponent;

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

	/** 初始化动画处理组件并绑定运动分析组件事件。 */
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

protected:
	/** 将运动信号转换为动画播放信号，蓝图可重写以接入职业、武器、Buff 等配置。 */
	UFUNCTION(BlueprintNativeEvent, Category="角色动画|动画处理", DisplayName="转换为动画信号")
	FLxCharacterAnimationSignal ConvertMotionSignalToAnimationSignal(const FLxCharacterMotionSignal& InMotionSignal) const;
	virtual FLxCharacterAnimationSignal ConvertMotionSignalToAnimationSignal_Implementation(const FLxCharacterMotionSignal& InMotionSignal) const;

private:
	void CacheOwnerComponents();
	void BindMotionAnalysisEvent();
	void UnbindMotionAnalysisEvent();
	void EnsureAnimationInstanceCached();

	/** 当前角色的运动分析组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterAnimationMotionAnalysisComponent> MotionAnalysisComponent = nullptr;

	/** 当前角色网格使用的动画实例。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxAnimInstanceBase> AnimInstance = nullptr;

	/** 最近一次生成的基础动画播放信号。 */
	UPROPERTY(Transient)
	FLxCharacterAnimationSignal CurrentBaseAnimationSignal;

	/** 最近一次生成的动作动画播放信号。 */
	UPROPERTY(Transient)
	FLxCharacterAnimationSignal CurrentActionAnimationSignal;

	bool bAnimationProcessInitialized = false;
};
