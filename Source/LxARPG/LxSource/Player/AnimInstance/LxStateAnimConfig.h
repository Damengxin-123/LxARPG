// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Characters/LxCharacterStateEnum.h"
#include "UObject/Object.h"
#include "LxStateAnimConfig.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FLxStateAnimConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName="角色状态")
	ELxCharacterState State;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName="动画序列")
	TObjectPtr<UAnimSequence> AnimSequence;
};
