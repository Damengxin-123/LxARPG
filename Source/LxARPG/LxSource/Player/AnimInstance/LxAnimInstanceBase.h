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
