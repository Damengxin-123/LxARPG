#pragma once

#include "CoreMinimal.h"
#include "LxInteractionEnum.generated.h"

UENUM(BlueprintType)
enum class ELxInteractionActionType : uint8
{
	Dialogue UMETA(DisplayName="对话交互"),
	TriggerMechanism UMETA(DisplayName="触发机关"),
	GiveItems UMETA(DisplayName="获取物品"),
	ConsumeItems UMETA(DisplayName="消耗物品"),
	TakeOnlyContainer UMETA(DisplayName="只取容器"),
	DepositAndTakeContainer UMETA(DisplayName="存取容器"),
	TradeContainer UMETA(DisplayName="交易容器"),
	FunctionPage UMETA(DisplayName="功能类"),
};

UENUM(BlueprintType)
enum class ELxInteractionPromptTextID : uint8
{
	OpenDoor UMETA(DisplayName="开门"),
	CloseDoor UMETA(DisplayName="关门"),
	StartElevator UMETA(DisplayName="启动电梯"),
	Dialogue UMETA(DisplayName="对话"),
	Pickup UMETA(DisplayName="拾取"),
};

UENUM(BlueprintType)
enum class ELxInteractionDataState : uint8
{
	Interactable UMETA(DisplayName="可交互"),
	Interacting UMETA(DisplayName="交互中"),
	Occupied UMETA(DisplayName="被占用"),
	Finished UMETA(DisplayName="交互结束"),
	NotInteractable UMETA(DisplayName="不可交互"),
};

UENUM(BlueprintType)
enum class ELxMechanismState : uint8
{
	Closed UMETA(DisplayName="关闭"),
	Opened UMETA(DisplayName="开启"),
	CannotOpen UMETA(DisplayName="不可开启"),
};

UENUM(BlueprintType)
enum class ELxFunctionPageID : uint8
{
	EquipmentEnhancement UMETA(DisplayName="强化装备页面"),
	ItemCrafting UMETA(DisplayName="制作物品页面"),
};
