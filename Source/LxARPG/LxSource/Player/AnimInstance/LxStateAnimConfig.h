// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Characters/LxCharacterStateEnum.h"
#include "UObject/Object.h"
#include "LxStateAnimConfig.generated.h"

/** 角色状态对应的动画序列配置。 */
USTRUCT(BlueprintType, DisplayName="状态动画配置")
struct FLxStateAnimConfig
{
	GENERATED_BODY()

	/** 该动画对应的角色状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="状态动画", DisplayName="角色状态")
	ELxCharacterState State = ELxCharacterState::Idle;

	/** 进入状态后使用的动画序列。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="状态动画", DisplayName="动画序列")
	TObjectPtr<UAnimSequence> AnimSequence;
};
