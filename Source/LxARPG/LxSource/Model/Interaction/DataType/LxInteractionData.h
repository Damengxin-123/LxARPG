#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxInteractionData.generated.h"

/** 属性类交互需求，例如力量达到指定值后才能交互。 */
USTRUCT(BlueprintType, DisplayName="交互属性需求")
struct FLxInteractionAttributeRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="属性ID")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	/** 交互需求的属性标签 ID，用于按 GameplayTag 判断角色属性是否达标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="属性ID标签", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="最小值")
	float MinValue = 0.0f;
};

/** 交互行为的通用需求集合，具体检测由功能组件执行。 */
USTRUCT(BlueprintType, DisplayName="交互需求")
struct FLxInteractionRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="所需物品列表")
	TArray<FLxItemQuote> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="所需属性列表")
	TArray<FLxInteractionAttributeRequirement> RequiredAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|需求", DisplayName="所需状态标签")
	FGameplayTagContainer RequiredStateTags;
};

/** 交互数据基础结构，用于保存类型、提示文本标签、状态和需求。 */
USTRUCT(BlueprintType, DisplayName="交互数据")
struct FLxInteractionDataBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互", DisplayName="交互类型")
	ELxInteractionActionType InteractionType = ELxInteractionActionType::Dialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互", DisplayName="提示文本标签")
	FGameplayTag PromptTextTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互", DisplayName="交互状态")
	ELxInteractionDataState InteractionState = ELxInteractionDataState::Interactable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互", DisplayName="交互需求")
	FLxInteractionRequirement Requirement;
};

/** 机关状态和该状态下提示文本标签的映射。 */
USTRUCT(BlueprintType, DisplayName="机关状态提示文本")
struct FLxMechanismStatePromptText
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|机关", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|机关", DisplayName="提示文本标签")
	FGameplayTag PromptTextTag;
};

/** 机关类交互数据。 */
USTRUCT(BlueprintType, DisplayName="触发机关交互数据")
struct FLxTriggerMechanismInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTriggerMechanismInteractionData()
	{
		InteractionType = ELxInteractionActionType::TriggerMechanism;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|机关", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|机关", DisplayName="机关状态提示文本")
	TArray<FLxMechanismStatePromptText> StatePromptTexts;
};

/** 对话类交互数据。 */
USTRUCT(BlueprintType, DisplayName="对话交互数据")
struct FLxDialogueInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxDialogueInteractionData()
	{
		InteractionType = ELxInteractionActionType::Dialogue;
	}
};

/** 只能从容器取出物品的宝箱交互数据。 */
USTRUCT(BlueprintType, DisplayName="宝箱交互数据")
struct FLxTreasureChestInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTreasureChestInteractionData()
	{
		InteractionType = ELxInteractionActionType::TreasureChest;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|宝箱", DisplayName="宝箱槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> ContainerSlotList;
};

/** 可以存入和取出物品的仓库交互数据。 */
USTRUCT(BlueprintType, DisplayName="仓库交互数据")
struct FLxWarehouseInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxWarehouseInteractionData()
	{
		InteractionType = ELxInteractionActionType::Warehouse;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|仓库", DisplayName="仓库槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> ContainerSlotList;
};

/** 交易容器中的单个槽位及其代价。 */
USTRUCT(BlueprintType, DisplayName="交易容器槽位")
struct FLxTradeContainerSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|交易", DisplayName="容器槽位")
	TObjectPtr<ULxItemSlotData> ContainerSlot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|交易", DisplayName="交易代价物品列表")
	TArray<FLxItemQuote> CostItemList;
};

/** 带代价的容器交互数据，例如商店。 */
USTRUCT(BlueprintType, DisplayName="交易容器交互数据")
struct FLxTradeContainerInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTradeContainerInteractionData()
	{
		InteractionType = ELxInteractionActionType::TradeContainer;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|交易", DisplayName="交易容器槽位列表")
	TArray<FLxTradeContainerSlot> TradeContainerSlotList;
};

/** 打开指定功能页面的交互数据。 */
USTRUCT(BlueprintType, DisplayName="功能界面交互数据")
struct FLxFunctionInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxFunctionInteractionData()
	{
		InteractionType = ELxInteractionActionType::FunctionPage;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互|功能", DisplayName="功能页面ID")
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
};
