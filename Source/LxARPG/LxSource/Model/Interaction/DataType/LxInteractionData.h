#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxInteractionData.generated.h"

/** 属性类交互需求，例如力量达到指定值后才能交互。 */
USTRUCT(BlueprintType, DisplayName = "交互属性需求")
struct FLxInteractionAttributeRequirement
{
	GENERATED_BODY()

	/** 交互需求的属性标签 ID，用于按 GameplayTag 判断角色属性是否达标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "属性标签ID", meta = (Categories = "属性"))
	FGameplayTag AttributeIDTag;

	/** 允许交互所需的最小属性值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "最小值")
	float MinValue = 0.0f;
};

/** 单个任务的状态限制，同一任务可允许多个状态。 */
USTRUCT(BlueprintType, DisplayName="交互任务状态需求")
struct FLxInteractionQuestRequirement
{
	GENERATED_BODY()

	/** 需要检查的任务所属系列。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="任务系列ID", meta=(Categories="任务"))
	FGameplayTag QuestSeriesId;

	/** 需要检查的任务标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="任务ID", meta=(Categories="任务"))
	FGameplayTag QuestId;

	/** 当前任务匹配任一状态即可；空列表不满足条件，默认要求任务已完成。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="允许的任务状态")
	TArray<ELxQuestRuntimeState> AllowedStates = {ELxQuestRuntimeState::Completed};
};

/** 交互行为的通用需求集合，由交互节点统一检查。 */
USTRUCT(BlueprintType, DisplayName = "交互需求")
struct FLxInteractionRequirement
{
	GENERATED_BODY()

	/** 所需物品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "所需物品列表")
	TArray<FLxItemQuote> RequiredItems;

	/** 所需属性列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "所需属性列表")
	TArray<FLxInteractionAttributeRequirement> RequiredAttributes;

	/** 所需状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "所需状态标签")
	FGameplayTagContainer RequiredStateTags;

	/** 所有任务条件均满足时才显示并允许执行节点；空列表表示不限制任务状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="任务状态限制")
	TArray<FLxInteractionQuestRequirement> RequiredQuests;
};

/** 机关状态和该状态下提示文本的映射。 */
USTRUCT(BlueprintType, DisplayName = "机关状态提示文本")
struct FLxMechanismStatePromptText
{
	GENERATED_BODY()

	/** 机关状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "提示文本")
	FText PromptText;
};

/** 宝箱功能模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "宝箱功能配置")
struct FLxTreasureChestInteractionConfig
{
	GENERATED_BODY()

	/** 宝箱初始化时创建的物品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|宝箱", DisplayName = "宝箱内物品列表")
	TArray<FLxItemQuote> ItemList;

	/** 取走多少个配置项目后视为完成；0 表示全部取走。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|宝箱", DisplayName = "获取完成限定数量", meta = (ClampMin = "0"))
	int32 AcquireCompletionLimit = 0;
};

/** 仓库功能模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "仓库功能配置")
struct FLxWarehouseInteractionConfig
{
	GENERATED_BODY()

	/** 仓库初始化时创建的槽位数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|仓库", DisplayName = "仓库槽位数量", meta = (ClampMin = "1"))
	int32 SlotCount = 100;
};

/** 商城功能模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "商城商品配置")
struct FLxTradeItemConfig
{
	GENERATED_BODY()

	/** 商城槽位中显示和交易的物品ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "物品ID", meta = (Categories = "物品"))
	FGameplayTag ItemIDTag;

	/** 当前一次购买会取得的物品数量，也是有限库存的初始数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "物品数量", meta = (ClampMin = "1", UIMin = "1"))
	int32 ItemCount = 1;

	/** 启用后购买成功会扣除商城槽位库存；关闭后商品可以无限次购买。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "有限库存")
	bool bLimitedStock = true;

	/** 转换为物品系统使用的物品引用。 */
	FLxItemQuote ToItemQuote() const { return FLxItemQuote(ItemIDTag, FMath::Max(1, ItemCount)); }
};

/** 商城功能模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "商城功能配置")
struct FLxTradeContainerInteractionConfig
{
	GENERATED_BODY()

	/** 商城初始化时创建的商品列表，每项可以分别控制是否为有限库存。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "商城商品列表")
	TArray<FLxTradeItemConfig> TradeItems;

	/** 旧商城商品列表，仅用于读取并兼容尚未迁移的蓝图数据。 */
	UPROPERTY(BlueprintReadOnly, Category = "交互|商城")
	TArray<FLxItemQuote> ItemList;

	/** 交易使用的金币物品标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "金币物品ID", meta = (Categories = "物品"))
	FGameplayTag GoldItemIDTag = LxTag_Item_Material_Currency_Gold;

	/** 商城向玩家出售物品时使用的价值倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "售卖价值比例", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SellValueRate = 1.0f;

	/** 商城从玩家处收购物品时使用的价值倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "收购价值比例", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PurchaseValueRate = 1.0f;
};

/** 机关功能模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "机关功能配置")
struct FLxTriggerMechanismInteractionConfig
{
	GENERATED_BODY()

	/** 机关初始化后的状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "初始机关状态")
	ELxMechanismState InitialState = ELxMechanismState::Closed;

	/** 不同机关状态对应的交互提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "机关状态提示文本")
	TArray<FLxMechanismStatePromptText> StatePromptTexts;
};

/** 物品传递功能模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "物品传递功能配置")
struct FLxItemTransferInteractionConfig
{
	GENERATED_BODY()

	/** 需要给予玩家或从玩家处移除的物品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|物品传递", DisplayName = "物品传递列表")
	TArray<FLxItemQuote> ItemList;

	/** 当前物品传递的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|物品传递", DisplayName = "物品传递方向")
	ELxItemTransferDirection Direction = ELxItemTransferDirection::AddToPlayer;
};

/** 功能界面模块的初始配置。 */
USTRUCT(BlueprintType, DisplayName = "功能界面配置")
struct FLxFunctionPageInteractionConfig
{
	GENERATED_BODY()

	/** 功能节点被选择后需要打开的功能页面。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能界面", DisplayName = "功能页面ID")
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
};

/** 单个任务交互节点使用的任务标识配置。 */
USTRUCT(BlueprintType, DisplayName="任务交互配置")
struct FLxQuestInteractionConfig
{
	GENERATED_BODY()

	/** 开启时使用任务可视化名称，关闭时使用节点自定义交互提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|任务", DisplayName="使用任务可视化文本")
	bool bUseQuestDisplayText = true;

	/** 当前交互节点引用的任务系列标签ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|任务", DisplayName="任务系列ID", meta=(Categories="任务"))
	FGameplayTag QuestSeriesId;

	/** 当前交互节点引用的任务标签ID，必须是任务系列ID的严格子标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|任务", DisplayName="任务ID", meta=(Categories="任务"))
	FGameplayTag QuestId;

	/** 判断系列ID和任务ID是否组成有效的父子标签关系。 */
	bool IsValid() const
	{
		return QuestSeriesId.IsValid() && QuestId.IsValid()
			&& QuestSeriesId != QuestId && QuestId.MatchesTag(QuestSeriesId);
	}
};
