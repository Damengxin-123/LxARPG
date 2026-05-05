// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxBuffEnum.generated.h"



UENUM(BlueprintType)
enum class ELxBuffType : uint8
{
	None UMETA(DisplayName="无类型"),
	Permanent UMETA(DisplayName="常驻型"),
	Timed UMETA(DisplayName="时效型"),
	AttributeGain UMETA(DisplayName="属性增益型"),
	AttributeRecovery UMETA(DisplayName="属性恢复型"),

	// Legacy values kept for existing data assets.
	AttributeModify UMETA(Hidden, DisplayName="属性修改型"),
	StateModify UMETA(Hidden, DisplayName="状态修改型"),
};
