#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "LxItemEnmuType.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ShowInfoConfig/LxItemRarityType.h"
#include "LxItemInformationBase.generated.h"

/**
 * 物品静态信息基础行。
 *
 * 所有具体物品数据表都继承该结构，运行时创建物品对象时会先按物品ID查询这些静态配置。
 */
USTRUCT(BlueprintType, DisplayName="物品基础信息")
struct FLxItemInformationBase : public FTableRowBase
{
	GENERATED_BODY()

	/** Stable item identity. Runtime lookup, stacking, and save-facing references all use this tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品ID", meta=(Categories="物品"))
	FGameplayTag ItemIDTag;

	/** 物品大类，例如装备、消耗品、材料、Buff。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品类型")
	ELxItemType ItemType = ELxItemType::None;

	/** 最大堆叠数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="最大堆叠数量")
	int32 ItemCountMax = 99;

	/** 当前物品数量，数据表中通常保持为0，由运行时物品对象写入实际数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品数量")
	int32 ItemCount = 0;

	/** 物品稀有度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品稀有度")
	ELxItemRarityType ItemRarity = ELxItemRarityType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品价值-金币",  meta=(ClampMin="0", UIMin="0"))
	int32 ItemSellPrice;

	/** 物品在 UI 中显示的名称，统一从物品基础数据读取。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|可视化", DisplayName="可视化名称")
	FText ItemDisplayName;

	/** 物品在 UI 中显示的描述，Tooltip 等界面直接读取该字段。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|可视化", DisplayName="可视化描述")
	FText ItemDisplayDescription;

	/** 物品图标，格子、拖拽图标和详情 UI 都从这里取图。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|可视化", DisplayName="图标")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	/** 物品自带词条引用，创建运行时物品对象后会初始化为词条对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品词条")
	TArray<FLxEntryQuote> ItemEntryQuotes;
};

/**
 * 运行时物品引用。
 *
 * 用于表达“某个标签 ID 的若干个物品”，例如背包添加、掉落生成和 Buff 创建词条。
 */
USTRUCT(BlueprintType, DisplayName="物品引用")
struct FLxItemQuote
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品ID", meta=(Categories="物品"))
	FGameplayTag ItemIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品", DisplayName="物品数量")
	int32 ItemCount = 0;

	FLxItemQuote() {}

	FLxItemQuote(FGameplayTag InItemIDTag, int32 InItemCount)
		: ItemIDTag(InItemIDTag)
		, ItemCount(InItemCount)
	{
	}

};
