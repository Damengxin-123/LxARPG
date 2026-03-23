// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxBuffEnum.generated.h"

/**
 * @enum ELxBuffType
 * @brief Buff类型。
 *
 * 该枚举用于区分 Buff 类物品或效果的子类型。
 * 当前仅保留默认占位值，便于后续扩展。
 *
 * @note 可在蓝图中使用。
 */
UENUM(BlueprintType)
enum class ELxBuffType : uint8
{
	/** 无 */
	None,
};
