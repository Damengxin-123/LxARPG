// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemSlotEnum.generated.h"


/**
 * @enum ELxItemSlotType
 * @brief 容器类型
 *
 * 定义了游戏中不同类型的物品容器。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType, DisplayName="物品槽位类型")
enum class ELxItemSlotType  : uint8
{
	// 空类型
	None			UMETA(DisplayName = "其他类型"),
	// 背包容器
	Backpack		UMETA(DisplayName = "背包容器"),
	// 装备栏容器
	Equipment		UMETA(DisplayName = "装备栏容器"),
	// 仓库容器（可放入物品）
	Warehouse 		UMETA(DisplayName = "仓库容器（可放入物品）"),
	// 箱子容器（不可放入物品）
	TreasureChest 	UMETA(DisplayName = "箱子容器（不可放入物品）"),
	// 交易容器（不能交换位置）
	Transaction 	UMETA(DisplayName = "交易容器（不能交换位置）"),
	// 快捷方式（只进不出）
	Shortcut  		UMETA(DisplayName = "快捷方式（只进不出）"),
};

