// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxItemEntryEnum.generated.h"

/**
 * @enum ELxItemEntryType
 * @brief 物品条目类型
 *
 * 该枚举定义了物品条目的不同种类，用于在系统中区分和处理各种类型的物品条目。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType, DisplayName="物品条目类型")
enum class ELxItemEntryType : uint8
{
	/**
	 * 基础数值型 - 表示为可以直接加算到角色属性值上
	 * 当值为此类型时，计算时直接加减即可
	 */
	BasicValue			UMETA(DisplayName = "基础数值型"),
	/**
	 * 提高基础型 - 表示可以提高基础值的百分比，仅用于基础属性
	 * 当值为此类型时，计算时需要按照百分比值计算，即 /100
	 */
	BasicImprove		UMETA(DisplayName = "提高基础型"),
	/**
	 * 额外提高型 - 表示在当前属性计算结果上，再次进行百分比提高，作用于当前属性数值
	 * 当值为此类型时，计算时需要按照百分比值计算，即 /100
	 */
	AdditionalImprove	UMETA(DisplayName = "额外提高型"),
	/**
	 * 机制型 - 表示为角色开启或关闭某些机制
	 * 当值为此类型时，视为布尔值，按照设定，当所有数值都为true时，则最终为true，否则为false
	 */
	Mechanism			UMETA(DisplayName = "机制型"),
};
