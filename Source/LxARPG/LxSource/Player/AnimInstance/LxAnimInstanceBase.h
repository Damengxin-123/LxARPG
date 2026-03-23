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
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="角色状态", DisplayName="角色状态")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="动画配置", DisplayName="动画配置")
	TArray<FLxStateAnimConfig> m_vStateAnimConfigs;

	UFUNCTION(BlueprintCallable, Category="动画", DisplayName="获取状态动画")
	UAnimSequence* GetAnimSequence(ELxCharacterState InState) const;
	
protected:
	// 持有此动画蓝图的角色
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pCharacter;

	// 状态动画缓存表
	UPROPERTY()
	TMap<ELxCharacterState, UAnimSequence*> m_mapStateAnimMap;
};
