// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryEnum.generated.h"

/**
 * @brief 物品词条加成方式枚举
 *
 * 该枚举定义了物品词条的不同加成方式，用于指定如何应用加成效果。每种加成方式对应不同的计算逻辑和应用顺序。
 * 在处理词条加成时，系统会根据此枚举确定当前词条的加成计算方法及其应用顺序。
 */
UENUM(BlueprintType, DisplayName="物品词条加成方式")
enum class ELxItemEntryType : uint8
{
	// 基础数值型
	BasicValue			UMETA(DisplayName = "基础数值型"),
	// 提高基础型
	BasicImprove		UMETA(DisplayName = "提高基础型"),
	// 额外提高型
	AdditionalImprove	UMETA(DisplayName = "额外提高型"),
	// 机制型
	Mechanism			UMETA(DisplayName = "机制型"),
};

/**
 * @brief 物品词条加成对象枚举
 *
 * 该枚举定义了物品词条加成效果的作用对象，用于指定加成应用的具体属性。每种加成对象对应不同的属性，系统会根据此枚举确定当前词条的加成作用于哪个属性上。
 * 支持的加成对象包括数值上限、当前有效值、向上浮动比例和向下浮动比例。
 */
UENUM(BlueprintType, DisplayName="物品词条加成对象")
enum class ELxItemEntryTarget : uint8
{
	// 作用于值的上限
	ToValueLimit				UMETA(DisplayName = "作用于值的上限"),
	// 作用于有效值
	ToValue						UMETA(DisplayName = "作用于有效值"),
	// 作用于向上浮动比例
	ToUpwardFloatingRatio		UMETA(DisplayName = "作用于向上浮动比例"),
	// 作用于向下浮动比例
	ToDownwardFloatingRatio		UMETA(DisplayName = "作用于向下浮动比例"),
};

/**
 * @brief 词条功能类型枚举
 *
 * 该枚举定义了不同类型的词条逻辑，用于指定词条在游戏中的具体功能。每种词条类型对应不同的行为和效果。
 * 在处理词条时，系统会根据此枚举确定当前词条的功能类型及其应用方式。
 */
UENUM(BlueprintType, DisplayName="词条功能类型")
enum class ELxEntryLogicType : uint8
{
	// 更改角色属性类型
	ChangeAttributeValue				UMETA(DisplayName="更改角色属性类型"),
	// 更改角色状态类型
	ChangeCharacterState				UMETA(DisplayName="更改角色状态类型"),
	// 创建buff类型
	CreaterBuffer						UMETA(DisplayName="创建buff类型"),
	// buff词条
	BufferEnter							UMETA(DisplayName="buff词条"),
	// 物品描述词条
	TextEnter							UMETA(DisplayName="描述文本类型词条"),
};

