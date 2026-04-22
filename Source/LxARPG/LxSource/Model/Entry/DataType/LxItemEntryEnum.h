// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryEnum.generated.h"

/**
 * @enum ELxItemEntryType
 * @brief 定义了物品词条加成的不同方式。
 *
 * 该枚举用于标识不同类型的物品属性加成，包括基础数值型、提高基础型、额外提高型和机制型。
 * 每种类型对应不同的加成逻辑，适用于游戏中的角色或装备属性调整。
 *
 * @var Mechanism
 * 机制型：设定一个最低值，保证目标属性至少达到这个值。
 */
UENUM(BlueprintType, DisplayName="物品词条加成方式")
enum class ELxItemEntryType : uint8
{
	// 基础数值型：直接增加一个固定数值到目标属性上。
	BasicValue			UMETA(DisplayName = "基础数值型"),
	// 提高基础型：基于基础值的一个百分比来增加目标属性。
	BasicImprove		UMETA(DisplayName = "提高基础型"),
	// 额外提高型：基于当前总值的一个百分比来进一步增加目标属性。
	AdditionalImprove	UMETA(DisplayName = "额外提高型"),
	// 机制型：设定某种机制是开启还是关闭，目前设定是关闭优先于开启。
	Mechanism			UMETA(DisplayName = "机制型"),
};

/**
 * @enum ELxItemEntryLogicType
 * @brief 定义了物品词条的不同逻辑类型。
 *
 * 该枚举用于标识物品词条在游戏中的不同逻辑行为，包括无逻辑、作用属性、提供Buff、即时恢复、增加状态、探索工具和投掷消耗等。
 * 每种逻辑类型对应不同的游戏内效果，适用于描述物品的具体功能。
 */
UENUM(BlueprintType, DisplayName="物品词条逻辑类型")
enum class ELxItemEntryLogicType : uint8
{
	// 无逻辑：表示该词条没有特定的逻辑行为。
	None							UMETA(DisplayName = "无逻辑"),
	// 作用于角色属性：该词条会修改角色的某个属性值。
	AttributeModifier				UMETA(DisplayName = "作用于角色属性"),
	// 作用于装备属性：该词条会修改角装备的某个属性值。
	AttributeModEquipment			UMETA(DisplayName = "作用于装备属性"),
	// 提供Buff：该词条会为角色或装备提供一个增益效果。
	ApplyBuff						UMETA(DisplayName = "提供Buff"),
	// 即时恢复：该词条会在使用时立即恢复某种资源（如生命值、魔力值等）。
	InstantRestore					UMETA(DisplayName = "即时恢复"),
	// 增加状态：该词条会为角色或装备增加某种状态效果。
	ApplyStatus						UMETA(DisplayName = "增加状态"),
	// 探索工具：该词条会为角色提供探索相关的辅助功能。
	ExploreUtility					UMETA(DisplayName = "探索工具"),
	// 投掷消耗：该词条允许物品被投掷并消耗以产生效果。
	ThrowConsumable					UMETA(DisplayName = "投掷消耗"),
};

/**
 * @enum ELxItemEntryTarget
 * @brief 定义了物品词条加成的作用对象。
 *
 * 该枚举用于标识物品属性加成所作用的具体目标，包括值的上限、有效值、向上浮动比例和向下浮动比例。
 * 每种类型对应不同的加成逻辑，适用于游戏中的角色或装备属性调整。
 * 
 */
UENUM(BlueprintType, DisplayName="物品词条加成对象")
enum class ELxItemEntryTarget : uint8
{
	// 作用于值的上限：加成作用于目标属性的最大值。
	ToValueLimit				UMETA(DisplayName = "作用于值的上限"),
	// 作用于有效值：加成直接作用于目标属性的有效值。
	ToValue						UMETA(DisplayName = "作用于有效值"),
	// 作用于向上浮动比例：加成作用于目标属性的向上浮动比例。
	ToUpwardFloatingRatio		UMETA(DisplayName = "作用于向上浮动比例"),
	// 作用于向下浮动比例：加成作用于目标属性的向下浮动比例。
	ToDownwardFloatingRatio		UMETA(DisplayName = "作用于向下浮动比例"),
};
