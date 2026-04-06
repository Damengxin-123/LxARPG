// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemEnmuType.h"
#include "UObject/Object.h"
#include "LxItemCoreType.generated.h"

USTRUCT(BlueprintType, DisplayName="物品基础信息")
struct FLxItemInfo
{
	GENERATED_BODY()

	/**
	 * @brief 物品唯一ID
	 * 该变量定义了物品的唯一标识符，用于在游戏内区分不同的物品。默认值为"null"，表示未指定的物品。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品唯一ID")
	FName ItemID = TEXT("null");

	/**
	 * @brief 物品类型
	 * 该变量定义了物品的类型，用于在游戏中分类不同的物品。默认值为`ELxItemType::None`，表示未指定或未知类型的物品。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品类型")
	ELxItemType ItemType = ELxItemType::None;
};

USTRUCT(BlueprintType, DisplayName="物品堆叠信息")
struct FLxItemStackInfo
{
	GENERATED_BODY()
	/**
	  * @brief 物品最大堆叠数量
	  * 该变量定义了单个物品类型在库存中可以堆叠的最大数量。默认值为99，表示每种物品最多可以堆叠99个。
	  * 可以在编辑器中设置，并且可以通过蓝图读写。
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品最大堆叠数量")
	int32 ItemMaxCount = 99;

	/**
	 * @brief 物品是否可以堆叠
	 * 该布尔变量用于指示物品是否可以在库存中堆叠。如果设置为`true`，则相同类型的物品可以堆叠在一起以节省空间；如果设置为`false`，则每个物品都必须单独占用一个槽位。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品是否可以堆叠")
	bool ItemCanStack = true;
};

USTRUCT(BlueprintType, DisplayName="物品可视化信息")
struct FLxItemShowInfo
{
	GENERATED_BODY()
	/**
	  * @brief 物品图标
	  * 该变量用于存储物品图标的引用，可以在编辑器中设置，并且支持蓝图读写。
	  * 通过此变量可以访问和显示物品的图标，以便在游戏中可视化地表示物品。
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品图标")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	/**
	 * @brief 物品可视化名称
	 * 该变量用于存储物品的显示名称，以便在游戏中显示给玩家。可以通过编辑器设置，并且支持蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品可视化名称")
	FText ItemNameText;

	/**
	 * @brief 物品可视化描述
	 * 该变量用于存储物品的详细描述文本，以便在游戏中显示给玩家。可以通过编辑器设置，并且支持蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品可视化描述")
	FText ItemDescription;
};
