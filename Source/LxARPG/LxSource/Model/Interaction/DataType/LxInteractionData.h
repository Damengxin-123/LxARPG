#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxInteractionData.generated.h"

USTRUCT(BlueprintType, DisplayName="交互属性需求")
struct FLxInteractionAttributeRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Requirement", DisplayName="属性ID")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Requirement", DisplayName="最小值")
	float MinValue = 0.0f;
};

USTRUCT(BlueprintType, DisplayName="交互需求")
struct FLxInteractionRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Requirement", DisplayName="所需物品列表")
	TArray<FLxItemQuote> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Requirement", DisplayName="所需属性列表")
	TArray<FLxInteractionAttributeRequirement> RequiredAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Requirement", DisplayName="所需状态标签")
	FGameplayTagContainer RequiredStateTags;
};

USTRUCT(BlueprintType, DisplayName="交互数据")
struct FLxInteractionDataBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", DisplayName="交互类型")
	ELxInteractionActionType InteractionType = ELxInteractionActionType::Dialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", DisplayName="交互文本ID")
	ELxInteractionPromptTextID PromptTextID = ELxInteractionPromptTextID::Dialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", DisplayName="交互状态")
	ELxInteractionDataState InteractionState = ELxInteractionDataState::Interactable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", DisplayName="交互需求")
	FLxInteractionRequirement Requirement;
};

USTRUCT(BlueprintType, DisplayName="机关状态提示文本")
struct FLxMechanismStatePromptText
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Mechanism", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Mechanism", DisplayName="提示文本ID")
	ELxInteractionPromptTextID PromptTextID = ELxInteractionPromptTextID::OpenDoor;
};

USTRUCT(BlueprintType, DisplayName="触发机关交互数据")
struct FLxTriggerMechanismInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTriggerMechanismInteractionData()
	{
		InteractionType = ELxInteractionActionType::TriggerMechanism;
		PromptTextID = ELxInteractionPromptTextID::OpenDoor;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Mechanism", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Mechanism", DisplayName="机关各状态提示文本")
	TArray<FLxMechanismStatePromptText> StatePromptTexts;
};

USTRUCT(BlueprintType, DisplayName="对话交互数据")
struct FLxDialogueInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxDialogueInteractionData()
	{
		InteractionType = ELxInteractionActionType::Dialogue;
		PromptTextID = ELxInteractionPromptTextID::Dialogue;
	}
};

USTRUCT(BlueprintType, DisplayName="获取物品交互数据")
struct FLxGiveItemsInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxGiveItemsInteractionData()
	{
		InteractionType = ELxInteractionActionType::GiveItems;
		PromptTextID = ELxInteractionPromptTextID::Pickup;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Item", DisplayName="物品列表")
	TArray<FLxItemQuote> ItemList;
};

USTRUCT(BlueprintType, DisplayName="消耗物品交互数据")
struct FLxConsumeItemsInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxConsumeItemsInteractionData()
	{
		InteractionType = ELxInteractionActionType::ConsumeItems;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Item", DisplayName="物品消耗列表")
	TArray<FLxItemQuote> ConsumedItemList;
};

USTRUCT(BlueprintType, DisplayName="只取容器交互数据")
struct FLxTakeOnlyContainerInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTakeOnlyContainerInteractionData()
	{
		InteractionType = ELxInteractionActionType::TakeOnlyContainer;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Container", DisplayName="容器槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> ContainerSlotList;
};

USTRUCT(BlueprintType, DisplayName="存取容器交互数据")
struct FLxDepositAndTakeContainerInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxDepositAndTakeContainerInteractionData()
	{
		InteractionType = ELxInteractionActionType::DepositAndTakeContainer;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Container", DisplayName="容器槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> ContainerSlotList;
};

USTRUCT(BlueprintType, DisplayName="交易容器槽位")
struct FLxTradeContainerSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Trade", DisplayName="容器槽位")
	TObjectPtr<ULxItemSlotData> ContainerSlot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Trade", DisplayName="交易代价物品列表")
	TArray<FLxItemQuote> CostItemList;
};

USTRUCT(BlueprintType, DisplayName="交易容器交互数据")
struct FLxTradeContainerInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxTradeContainerInteractionData()
	{
		InteractionType = ELxInteractionActionType::TradeContainer;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Trade", DisplayName="交易类容器槽位列表")
	TArray<FLxTradeContainerSlot> TradeContainerSlotList;
};

USTRUCT(BlueprintType, DisplayName="功能类交互数据")
struct FLxFunctionInteractionData : public FLxInteractionDataBase
{
	GENERATED_BODY()

	FLxFunctionInteractionData()
	{
		InteractionType = ELxInteractionActionType::FunctionPage;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Function", DisplayName="功能页面ID")
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
};
