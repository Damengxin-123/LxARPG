// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemCoreType.h"
#include "LxItemEnmuType.h"
#include "LxRarityInfoData.h"
#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "UObject/Object.h"
#include "LxItemBase.generated.h"


/**
 * 物品属性定义基类
 * 用于在设计物品时进行基本信息定义
 */
USTRUCT(BlueprintType, DisplayName="物品属性定义基础类型")
struct FLxItemDefineBase : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * @brief 物品基础信息
	 * 该变量包含了物品的基础属性，如唯一ID和类型等。用于在游戏中定义和区分不同的物品。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础信息", DisplayName="物品基础信息")
	FLxItemInfo ItemInfo;

	/**
	 * @brief 物品堆叠信息
	 * 该变量包含了物品堆叠相关的属性，如最大堆叠数量和是否可以堆叠。这些属性用于在游戏中管理库存中的物品堆叠行为。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品堆叠信息", DisplayName="物品堆叠信息")
	FLxItemStackInfo ItemStackInfo;

	/**
	 * @brief 物品可视化信息
	 * 该变量包含了物品的图标、名称和描述等可视化属性，用于在游戏中向玩家展示物品的相关信息。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品可视化信息")
	FLxItemShowInfo ItemShowInfo;

	/**
	 * @brief 物品稀有度数据表行句柄
	 * 该变量用于引用定义在LxRarityInfo数据表中的特定稀有度信息。通过此句柄，可以访问与物品稀有度相关的各种属性，如名称、颜色或掉落概率等。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品稀有度信息引用",  meta=(RowType="LxRarityInfo"))
	FDataTableRowHandle ItemRarityRowQuote;
};

/**
 * @brief 物品属性缓存类型
 * 该结构体用于存储和管理游戏中物品的各种属性信息，包括基础信息、堆叠信息、数量、可视化信息及稀有度等。通过将这些信息封装在一个结构体内，方便在游戏逻辑中进行传递和使用。
 * 所有成员变量均支持在编辑器内配置，并且可以通过蓝图脚本访问和修改，以实现更灵活的游戏设计。
 */
USTRUCT(BlueprintType, DisplayName="物品属性缓存类型")
struct FLxItemDateBase
{
	GENERATED_BODY()
	
	/**
 	 * @brief 物品基础信息
 	 * 该变量包含了物品的基础属性，如唯一ID和类型等。用于在游戏中定义和区分不同的物品。
 	 * 可以在编辑器中设置，并且可以通过蓝图读写。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础信息", DisplayName="物品基础信息")
	FLxItemInfo ItemInfo;

	/**
	 * @brief 物品堆叠信息
	 * 该变量包含了物品堆叠相关的属性，如最大堆叠数量和是否可以堆叠。这些属性用于在游戏中管理库存中的物品堆叠行为。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品堆叠信息", DisplayName="物品堆叠信息")
	FLxItemStackInfo ItemStackInfo;

	/**
	 * @brief 物品数量
	 * 该变量定义了当前物品的数量，用于在游戏中表示某个物品的具体数量。默认值为`ERR_ATTRIBUTE`（-9999），表示未指定或无效的物品数量。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品堆叠信息", DisplayName="物品数量")
	int32 ItemCount = ERR_ATTRIBUTE;

	/**
	 * @brief 物品可视化信息
	 * 该变量包含了物品的图标、名称和描述等可视化属性，用于在游戏中向玩家展示物品的相关信息。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品可视化信息")
	FLxItemShowInfo ItemShowInfo;

	/**
	 * @brief 物品稀有度信息
	 * 该变量定义了物品的稀有度相关属性，包括稀有度数值、稀有度描述和稀有度颜色。这些属性用于在游戏中表示物品的稀有程度，并在UI中进行可视化展示。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品稀有度信息")
	FLxRarityInfo ItemRarity;
};
