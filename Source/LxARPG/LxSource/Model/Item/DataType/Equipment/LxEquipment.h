// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxEquipmentEnum.h"
#include "../ItemBase/LxItemBase.h"
#include "LxEquipmentCoreType.h"
#include "LxEquipment.generated.h"


/**
 * @brief 装备属性定义类型
 *
 * 该结构体继承自`FLxItemDefineBase`，用于定义装备的属性。它扩展了基础物品定义，以包含特定于装备的信息。
 * 可以在编辑器中设置，并且可以通过蓝图读写。
 *
 * @see FLxItemDefineBase
 */
USTRUCT(BlueprintType, DisplayName="装备属性定义类型")
struct FLxEquipmentDefine : public FLxItemDefineBase
{
	GENERATED_BODY()

	/**
	 * @var FLxEquipmentInfo EquipmentInfo
	 * @brief 装备的基础默认属性信息
	 * 该变量用于存储装备的基础属性信息，包括装备部位类型、默认强化强度和默认锻造潜能等。这些属性在编辑器中可配置，并且可以通过蓝图脚本进行读写。
	 *
	 * @note 确保所有引用的词条定义存在并且有效，以避免运行时错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备属性", DisplayName="装备基础默认属性")
	FLxEquipmentInfo EquipmentInfo;

	/**
	 * @var FLxEquipmentEntyQuote EquipmentEntyQuoteInfo
	 * @brief 装备默认词条配置
	 * 该变量用于存储装备的默认词条引用信息，包括基础词条和扩展词条。通过这些引用，可以定义装备的基础属性和附加属性。
	 * 此属性支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作。
	 *
	 * @note 确保所引用的词条定义存在并且是有效的，以避免运行时可能出现的错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备属性", DisplayName="装备默认词条配置")
	FLxEquipmentEntyQuote EquipmentEntyQuoteInfo;

	// 构造函数，将部分属性设置为装备类型物品专属的
	FLxEquipmentDefine()
	{
		ItemInfo.ItemType = ELxItemType::Equipment; // 将物品类型设置为装备
		ItemStackInfo.ItemCanStack = false; // 装备通常不能堆叠
		ItemStackInfo.ItemMaxCount = 1; // 最大堆叠数量
	}
};

USTRUCT(BlueprintType, DisplayName="装备属性缓存类型")
struct FLxEquipmentData : public FLxItemDateBase
{
	GENERATED_BODY()
	/**
 	 * @var FLxEquipmentInfo EquipmentInfo
 	 * @brief 装备的基础默认属性信息
 	 * 该变量用于存储装备的基础属性信息，包括装备部位类型、默认强化强度和默认锻造潜能等。这些属性在编辑器中可配置，并且可以通过蓝图脚本进行读写。
 	 *
 	 * @note 确保所有引用的词条定义存在并且有效，以避免运行时错误。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备基础默认属性")
	FLxEquipmentInfo EquipmentInfo;

	/**
	 * @var FLxEquipmentEntyInfo EquipmentEntyInfo
	 * @brief 装备词条信息
	 * 该变量包含了装备的默认词条和扩展词条列表。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“装备词条信息”。
	 *
	 * @note 确保所有引用的词条定义存在并且有效，以避免运行时错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备词条信息")
	FLxEquipmentEntyInfo EquipmentEntyInfo;
};



