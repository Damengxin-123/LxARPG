// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxMaterialEnum.generated.h"

UENUM(BlueprintType)
enum class ELxMaterialType : uint8
{
	None = 0 UMETA(DisplayName="无类型"),
	Normal UMETA(DisplayName="普通材料"),
	Quest UMETA(DisplayName="任务物品"),
};
