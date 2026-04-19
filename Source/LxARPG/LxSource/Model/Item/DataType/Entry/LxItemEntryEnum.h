// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryEnum.generated.h"

UENUM(BlueprintType, DisplayName="物品词条加成方式")
enum class ELxItemEntryType : uint8
{
	BasicValue UMETA(DisplayName = "基础数值型"),
	BasicImprove UMETA(DisplayName = "提高基础型"),
	AdditionalImprove UMETA(DisplayName = "额外提高型"),
	Mechanism UMETA(DisplayName = "机制型"),
};

UENUM(BlueprintType, DisplayName="物品词条逻辑类型")
enum class ELxItemEntryLogicType : uint8
{
	None UMETA(DisplayName = "无逻辑"),
	AttributeModifier UMETA(DisplayName = "作用属性"),
	ApplyBuff UMETA(DisplayName = "提供Buff"),
	InstantRestore UMETA(DisplayName = "即时恢复"),
	ApplyStatus UMETA(DisplayName = "增加状态"),
	ExploreUtility UMETA(DisplayName = "探索工具"),
	ThrowConsumable UMETA(DisplayName = "投掷消耗"),
};

UENUM(BlueprintType, DisplayName="物品词条加成对象")
enum class ELxItemEntryTarget : uint8
{
	ToValueLimit UMETA(DisplayName = "作用于值的上限"),
	ToValue UMETA(DisplayName = "作用于有效值"),
	ToUpwardFloatingRatio UMETA(DisplayName = "作用于向上浮动比例"),
	ToDownwardFloatingRatio UMETA(DisplayName = "作用于向下浮动比例"),
};
