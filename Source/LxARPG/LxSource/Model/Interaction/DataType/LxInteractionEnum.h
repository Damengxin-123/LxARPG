#pragma once

#include "CoreMinimal.h"
#include "LxInteractionEnum.generated.h"

/** 交互行为的分类，用于节点、功能组件和UI之间做类型分发。 */
UENUM(BlueprintType)
enum class ELxInteractionActionType : uint8
{
	Dialogue UMETA(DisplayName="对话交互"),
	TriggerMechanism UMETA(DisplayName="触发机关"),
	GiveItems UMETA(DisplayName="获得物品"),
	ConsumeItems UMETA(DisplayName="失去物品"),
	TakeOnlyContainer UMETA(DisplayName="只取容器"),
	DepositAndTakeContainer UMETA(DisplayName="存取容器"),
	TradeContainer UMETA(DisplayName="交易容器"),
	FunctionPage UMETA(DisplayName="功能界面"),
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
