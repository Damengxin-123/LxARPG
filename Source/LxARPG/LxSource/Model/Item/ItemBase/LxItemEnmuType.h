// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxItemEnmuType.generated.h"

/**
 * @file LxItemEnmuType.h
 * @brief 物品相关枚举类型定义
 *
 * 此文件用于定义物品相关的枚举类型。
 */

/**
 * @enum ELxItemType
 * @brief 物品类型
 *
 * 定义了游戏中所有物品的大类类型。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType, DisplayName="物品类型")
enum class ELxItemType : uint8
{
	/** 其他类型 */
	None		UMETA(DisplayName = "其他类型"),
	/** 装备类型 */
	Equipment	UMETA(DisplayName = "装备"),
	/** 消耗品类型 */
	Consumable	UMETA(DisplayName = "消耗品"),
	/** 材料类型 */
	Material	UMETA(DisplayName = "材料"),
	/** 技能 */
	Skill		UMETA(DisplayName = "技能"),
	/** 状态 */
	Buff		UMETA(DisplayName = "状态"),
	
	/** 枚举最大值 - 此枚举表示物品类型有多少个类型 */
	ItemSizeMax UMETA(Hidden, DisplayName = "枚举最大值"),
};








