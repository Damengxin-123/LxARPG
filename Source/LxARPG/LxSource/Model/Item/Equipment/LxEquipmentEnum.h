// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxItemEntryEnum.generated.h"

/**
 * @enum ELxEquipmentType
 * @brief 装备类型
 *
 * 定义了装备的不同部位类型。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType, DisplayName="装备类型")
enum class ELxEquipmentType : uint8
{
	/** 武器部位 */
	Weapon		UMETA(DisplayName = "武器"),
	/** 副手部位 */
	Deputy		UMETA(DisplayName = "副手"),
	/** 头盔部位 */
	Helmet		UMETA(DisplayName = "头盔"),
	/** 胸甲部位 */
	Armor		UMETA(DisplayName = "胸甲"),
	/** 护腿部位 */
	Leggings	UMETA(DisplayName = "护腿"),
	/** 鞋子部位 */
	Boots		UMETA(DisplayName = "鞋子"),
	/** 手套部位 */
	Glove		UMETA(DisplayName = "手套"),
	/** 腰带部位 */
	Belt		UMETA(DisplayName = "腰带"),
	/** 饰品部位 */
	Jewelry		UMETA(DisplayName = "饰品"),
	/** 枚举最大值 - 此枚举表示装备位置有多少个类型 */
	EquipmentSizeMax UMETA(Hidden, DisplayName = "枚举最大值"),
};
