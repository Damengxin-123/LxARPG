// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LxStateAnimConfig.h"
#include "LxARPG/LxSource/Player/Characters/LxCharacterStateEnum.h"
#include "LxAnimInstanceBase.generated.h"

class ALxBaseCharacter;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="动画配置", DisplayName="动画配置")
	TArray<FLxStateAnimConfig> m_vStateAnimConfigs;

	UFUNCTION(BlueprintCallable, Category="动画", DisplayName="获取状态动画")
	/**
	 * @brief 根据角色状态获取对应动画序列。
	 *
	 * @param InState 目标角色状态。
	 * @return 返回该状态配置的动画序列，未找到时返回 nullptr。
	 */
	UAnimSequence* GetAnimSequence(ELxCharacterState InState) const;
	
protected:
	// 持有此动画蓝图的角色
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pCharacter;

	// 状态动画缓存表
	UPROPERTY()
	TMap<ELxCharacterState, UAnimSequence*> m_mapStateAnimMap;
};
