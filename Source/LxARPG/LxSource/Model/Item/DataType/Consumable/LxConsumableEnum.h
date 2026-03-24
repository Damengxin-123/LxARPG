// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxConsumableEnum.generated.h"


/**
 * @enum ELxConsumableType
 * @brief 消耗品类型。
 *
 * 该枚举用于区分不同的消耗品子类型（如恢复类、增益类等）。
 * 当前仅保留默认占位值，便于后续扩展。
 *
 * @note 可在蓝图中使用。
 */
UENUM(BlueprintType)
enum class ELxConsumableType : uint8
{
	/** 无 */
	None UMETA(DisplayName="无类型"),
	/** 可消耗型 即在使用之后数量会减少的消耗品 */
	Consumable UMETA(DisplayName="可消耗"),
	/** 不可消耗型 指在通常情况下数量不会直接减少的消耗品类型 */
	NotConsumable UMETA(DisplayName="不可消耗")
};
