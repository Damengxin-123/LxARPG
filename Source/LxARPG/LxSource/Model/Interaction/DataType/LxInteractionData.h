#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
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

/** 交互行为的通用需求集合，具体检测由功能组件执行。 */
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
};

/** 交互数据基础结构，用于保存类型、提示文本标签、状态和需求。 */
USTRUCT(BlueprintType, DisplayName = "交互数据")
struct FLxInteractionDataBase
{
	GENERATED_BODY()

	/** 交互类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互", DisplayName = "交互类型")
	ELxInteractionActionType InteractionType = ELxInteractionActionType::Dialogue;

	/** 提示文本标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互", DisplayName = "提示文本标签")
	FGameplayTag PromptTextTag;

	/** 交互状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互", DisplayName = "交互状态")
	ELxInteractionDataState InteractionState = ELxInteractionDataState::Interactable;

	/** 交互需求。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互", DisplayName = "交互需求")
	FLxInteractionRequirement Requirement;
};

/** 机关状态和该状态下提示文本标签的映射。 */
USTRUCT(BlueprintType, DisplayName = "机关状态提示文本")
struct FLxMechanismStatePromptText
{
	GENERATED_BODY()

	/** 机关状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 提示文本标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "提示文本标签")
	FGameplayTag PromptTextTag;
};

/** 触发机关类交互数据。 */
USTRUCT(BlueprintType, DisplayName = "触发机关交互数据")
struct FLxTriggerMechanismInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTriggerMechanismInteractionData()
	{
		InteractionType = ELxInteractionActionType::TriggerMechanism;
	}

	/** 机关状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 机关状态提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|机关", DisplayName = "机关状态提示文本")
	TArray<FLxMechanismStatePromptText> StatePromptTexts;
};

/** 对话类交互数据。 */
USTRUCT(BlueprintType, DisplayName = "对话交互数据")
struct FLxDialogueInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxDialogueInteractionData()
	{
		InteractionType = ELxInteractionActionType::Dialogue;
	}
};

/** 只能从容器取出物品的宝箱交互数据。 */
USTRUCT(BlueprintType, DisplayName = "宝箱交互数据")
struct FLxTreasureChestInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTreasureChestInteractionData()
	{
		InteractionType = ELxInteractionActionType::TreasureChest;
	}

	/** 宝箱槽位列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|宝箱", DisplayName = "宝箱槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> ContainerSlotList;
};

/** 可以存入和取出物品的仓库交互数据。 */
USTRUCT(BlueprintType, DisplayName = "仓库交互数据")
struct FLxWarehouseInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxWarehouseInteractionData()
	{
		InteractionType = ELxInteractionActionType::Warehouse;
	}

	/** 仓库槽位列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|仓库", DisplayName = "仓库槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> ContainerSlotList;
};

/** 交易容器中的单个槽位及其代价。 */
USTRUCT(BlueprintType, DisplayName = "交易容器槽位")
struct FLxTradeContainerSlot
{
	GENERATED_BODY()

	/** 容器槽位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|交易", DisplayName = "容器槽位")
	TObjectPtr<ULxItemSlotData> ContainerSlot = nullptr;

	/** 交易代价物品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|交易", DisplayName = "交易代价物品列表")
	TArray<FLxItemQuote> CostItemList;
};

/** 带代价的容器交互数据，例如商店。 */
USTRUCT(BlueprintType, DisplayName = "交易容器交互数据")
struct FLxTradeContainerInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTradeContainerInteractionData()
	{
		InteractionType = ELxInteractionActionType::TradeContainer;
	}

	/** 交易容器槽位列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|交易", DisplayName = "交易容器槽位列表")
	TArray<FLxTradeContainerSlot> TradeContainerSlotList;
};

/** 打开指定功能页面的交互数据。 */
USTRUCT(BlueprintType, DisplayName = "功能界面交互数据")
struct FLxFunctionInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxFunctionInteractionData()
	{
		InteractionType = ELxInteractionActionType::FunctionPage;
	}

	/** 功能页面 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能页面", DisplayName = "功能页面ID")
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
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
USTRUCT(BlueprintType, DisplayName = "商城功能配置")
struct FLxTradeContainerInteractionConfig
{
	GENERATED_BODY()

	/** 商城初始化时创建的商品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "商城商品列表")
	TArray<FLxItemQuote> ItemList;

	/** 交易使用的金币物品标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|商城", DisplayName = "金币物品ID", meta = (Categories = "物品"))
	FGameplayTag GoldItemIDTag;

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

/**
 * 功能节点可以使用的初始配置集合。
 * “显示配置”布尔值只整理编辑器细节面板，不参与功能启用和模块实例化判断。
 */
USTRUCT(BlueprintType, DisplayName = "交互功能节点配置")
struct FLxInteractionFeatureNodeConfig
{
	GENERATED_BODY()

	/** 是否在细节面板中显示宝箱配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|编辑器显示", DisplayName = "显示宝箱配置")
	bool bShowTreasureChestConfig = false;

	/** 宝箱功能的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能配置", DisplayName = "宝箱功能配置", meta = (EditCondition = "bShowTreasureChestConfig", EditConditionHides))
	FLxTreasureChestInteractionConfig TreasureChestConfig;

	/** 是否在细节面板中显示仓库配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|编辑器显示", DisplayName = "显示仓库配置")
	bool bShowWarehouseConfig = false;

	/** 仓库功能的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能配置", DisplayName = "仓库功能配置", meta = (EditCondition = "bShowWarehouseConfig", EditConditionHides))
	FLxWarehouseInteractionConfig WarehouseConfig;

	/** 是否在细节面板中显示商城配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|编辑器显示", DisplayName = "显示商城配置")
	bool bShowTradeContainerConfig = false;

	/** 商城功能的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能配置", DisplayName = "商城功能配置", meta = (EditCondition = "bShowTradeContainerConfig", EditConditionHides))
	FLxTradeContainerInteractionConfig TradeContainerConfig;

	/** 是否在细节面板中显示机关配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|编辑器显示", DisplayName = "显示机关配置")
	bool bShowTriggerMechanismConfig = false;

	/** 机关功能的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能配置", DisplayName = "机关功能配置", meta = (EditCondition = "bShowTriggerMechanismConfig", EditConditionHides))
	FLxTriggerMechanismInteractionConfig TriggerMechanismConfig;

	/** 是否在细节面板中显示物品传递配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|编辑器显示", DisplayName = "显示物品传递配置")
	bool bShowItemTransferConfig = false;

	/** 物品传递功能的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能配置", DisplayName = "物品传递功能配置", meta = (EditCondition = "bShowItemTransferConfig", EditConditionHides))
	FLxItemTransferInteractionConfig ItemTransferConfig;

	/** 是否在细节面板中显示功能界面配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|编辑器显示", DisplayName = "显示功能界面配置")
	bool bShowFunctionPageConfig = false;

	/** 功能界面节点的初始配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能配置", DisplayName = "功能界面配置", meta = (EditCondition = "bShowFunctionPageConfig", EditConditionHides))
	FLxFunctionPageInteractionConfig FunctionPageConfig;
};
