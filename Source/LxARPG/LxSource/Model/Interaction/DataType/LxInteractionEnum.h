#pragma once

#include "CoreMinimal.h"
#include "LxInteractionEnum.generated.h"

/** 交互行为的分类，用于节点、功能组件和UI之间做类型分发。 */
UENUM(BlueprintType)
enum class ELxInteractionActionType : uint8
{
	Entrance UMETA(DisplayName="交互入口"),
	Dialogue UMETA(DisplayName="对话交互"),
	TriggerMechanism UMETA(DisplayName="触发机关"),
	ItemTransfer UMETA(DisplayName="物品传递"),
	TreasureChest UMETA(DisplayName="宝箱"),
	Warehouse UMETA(DisplayName="仓库"),
	TradeContainer UMETA(DisplayName="交易容器"),
	FunctionPage UMETA(DisplayName="功能界面"),
	/** 交互出口节点，选择后关闭当前交互流程。 */
	InteractionExit UMETA(DisplayName="交互出口"),
};

/** 物品传递方向，表示对玩家角色背包增加或移除物品。 */
UENUM(BlueprintType)
enum class ELxItemTransferDirection : uint8
{
	AddToPlayer UMETA(DisplayName="增加到玩家"),
	RemoveFromPlayer UMETA(DisplayName="从玩家移除"),
};

/** 单个交互行为当前是否可用，不表示玩家交互流程生命周期。 */
UENUM(BlueprintType)
enum class ELxInteractionDataState : uint8
{
	Interactable UMETA(DisplayName="可交互"),
	Interacting UMETA(DisplayName="交互中"),
	Occupied UMETA(DisplayName="被占用"),
	Finished UMETA(DisplayName="交互结束"),
	NotInteractable UMETA(DisplayName="不可交互"),
};

/** 简单机关的运行状态。 */
UENUM(BlueprintType)
enum class ELxMechanismState : uint8
{
	Closed UMETA(DisplayName="关闭"),
	Opened UMETA(DisplayName="开启"),
	CannotOpen UMETA(DisplayName="不可开启"),
};

/** 功能类交互打开的页面类型。 */
UENUM(BlueprintType)
enum class ELxFunctionPageID : uint8
{
	EquipmentEnhancement UMETA(DisplayName="装备强化"),
	ItemCrafting UMETA(DisplayName="物品制作"),
};
