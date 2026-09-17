#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxInteractionData.generated.h"

class ULxUIBaseObject;

/** 单项物品限制，独立配置所需物品数量和生效方式。 */
USTRUCT(BlueprintType, DisplayName="交互物品需求")
struct FLxInteractionItemRequirement
{
	GENERATED_BODY()

	/** 需要持有的物品标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="物品ID", meta=(Categories="物品"))
	FGameplayTag ItemIDTag;

	/** 需要持有的物品数量，同类必要物品条件的数量累加。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="物品数量", meta=(ClampMin="1", UIMin="1"))
	int32 ItemCount = 1;

	/** 必要项全部满足；跨类别的可选项至少满足一项。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="生效方式")
	ELxInteractionRequirementMode Mode = ELxInteractionRequirementMode::Required;

	/** 转换为背包查询使用的物品引用。 */
	FLxItemQuote ToItemQuote() const { return FLxItemQuote(ItemIDTag, ItemCount); }
};

/** 单项角色状态标签限制。 */
USTRUCT(BlueprintType, DisplayName="交互状态需求")
struct FLxInteractionStateRequirement
{
	GENERATED_BODY()

	/** 角色需要持有的状态标签，保留标签层级匹配语义。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="状态标签")
	FGameplayTag StateTag;

	/** 必要项全部满足；跨类别的可选项至少满足一项。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="生效方式")
	ELxInteractionRequirementMode Mode = ELxInteractionRequirementMode::Required;
};

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

	/** 必要项全部满足；跨类别的可选项至少满足一项。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="生效方式")
	ELxInteractionRequirementMode Mode = ELxInteractionRequirementMode::Required;
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

	/** 必要项全部满足；跨类别的可选项至少满足一项。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="生效方式")
	ELxInteractionRequirementMode Mode = ELxInteractionRequirementMode::Required;
};

/** 交互行为的通用需求集合，由交互节点统一检查。 */
USTRUCT(BlueprintType, DisplayName = "交互需求")
struct LXARPG_API FLxInteractionRequirement
{
	GENERATED_BODY()

	/** 所需物品列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "所需物品列表")
	TArray<FLxInteractionItemRequirement> ItemRequirements;

	/** 所需属性列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "所需属性列表")
	TArray<FLxInteractionAttributeRequirement> RequiredAttributes;

	/** 所需状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|需求", DisplayName = "所需状态标签")
	TArray<FLxInteractionStateRequirement> StateRequirements;

	/** 每项检查一个任务的允许状态，并按生效方式与其他类别条件组合。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="任务状态限制")
	TArray<FLxInteractionQuestRequirement> RequiredQuests;

	/** 加载旧资产时，将旧物品列表和状态容器迁移为必要条件，并清空旧数据。 */
	void PostSerialize(const FArchive& Ar);

private:
	/** 仅用于读取旧版物品限制；新配置统一保存到 ItemRequirements。 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="使用逐项配置的物品限制"))
	TArray<FLxItemQuote> RequiredItems_DEPRECATED;

	/** 仅用于读取旧版状态限制；新配置统一保存到 StateRequirements。 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="使用逐项配置的状态限制"))
	FGameplayTagContainer RequiredStateTags_DEPRECATED;
};

/** 让嵌套在交互资产中的限制配置在反序列化后执行旧字段迁移。 */
template<>
struct TStructOpsTypeTraits<FLxInteractionRequirement> : TStructOpsTypeTraitsBase2<FLxInteractionRequirement>
{
	enum { WithPostSerialize = true };
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

	/** 选择功能节点后创建的自定义页面；留空时该节点不可交互。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能界面", DisplayName = "功能页面类")
	TSubclassOf<ULxUIBaseObject> PageWidgetClass;

	/** 功能节点被选择后需要打开的功能页面。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "交互|功能界面", DisplayName = "功能页面ID")
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
};

/** 每个NPC独立配置的功能开关与初始内容，运行时状态由功能模块持有。 */
USTRUCT(BlueprintType, DisplayName="角色交互功能配置")
struct FLxInteractableFeatureConfig
{
	GENERATED_BODY()

	/** 当前NPC是否启用宝箱功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用宝箱功能")
	bool bEnableTreasureChest = false;
	/** 当前NPC的宝箱初始内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="宝箱配置", meta=(EditCondition="bEnableTreasureChest", EditConditionHides))
	FLxTreasureChestInteractionConfig TreasureChestConfig;

	/** 当前NPC是否启用仓库功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用仓库功能")
	bool bEnableWarehouse = false;
	/** 当前NPC的仓库初始内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="仓库配置", meta=(EditCondition="bEnableWarehouse", EditConditionHides))
	FLxWarehouseInteractionConfig WarehouseConfig;

	/** 当前NPC是否启用交易功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用交易功能")
	bool bEnableTradeContainer = false;
	/** 当前NPC的交易初始内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="交易配置", meta=(EditCondition="bEnableTradeContainer", EditConditionHides))
	FLxTradeContainerInteractionConfig TradeContainerConfig;

	/** 当前NPC是否启用机关功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用机关功能")
	bool bEnableTriggerMechanism = false;
	/** 当前NPC的机关初始内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="机关配置", meta=(EditCondition="bEnableTriggerMechanism", EditConditionHides))
	FLxTriggerMechanismInteractionConfig TriggerMechanismConfig;

	/** 当前NPC是否启用物品传递功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用物品传递功能")
	bool bEnableItemTransfer = false;
	/** 当前NPC的物品传递初始内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="物品传递配置", meta=(EditCondition="bEnableItemTransfer", EditConditionHides))
	FLxItemTransferInteractionConfig ItemTransferConfig;

	/** 当前NPC是否启用功能界面功能。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用功能界面功能")
	bool bEnableFunctionPage = false;
	/** 当前NPC的功能界面初始内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="功能界面配置", meta=(EditCondition="bEnableFunctionPage", EditConditionHides))
	FLxFunctionPageInteractionConfig FunctionPageConfig;

	/** 当前NPC是否启用任务功能；具体任务仍由各任务节点引用。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="功能", DisplayName="启用任务功能")
	bool bEnableQuest = false;

	/** 查询当前NPC是否启用了指定功能。 */
	bool IsEnabled(ELxInteractionActionType Type) const
	{
		switch (Type)
		{
		case ELxInteractionActionType::TreasureChest: return bEnableTreasureChest;
		case ELxInteractionActionType::Warehouse: return bEnableWarehouse;
		case ELxInteractionActionType::TradeContainer: return bEnableTradeContainer;
		case ELxInteractionActionType::TriggerMechanism: return bEnableTriggerMechanism;
		case ELxInteractionActionType::ItemTransfer: return bEnableItemTransfer;
		case ELxInteractionActionType::FunctionPage: return bEnableFunctionPage;
		case ELxInteractionActionType::Quest: return bEnableQuest;
		default: return false;
		}
	}
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
