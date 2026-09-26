// Fill out your copyright notice in the Description page of Project Settings.


#include "LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxItemTransferInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxQuestInteractionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Chat/Logic/LxPlayerChatComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/SystemOperate/LxPlayerSystemOperateComponent.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

namespace
{
	/** 根据对象、资产实例版本和节点序号获取功能，拒绝树切换前发出的过期请求。 */
	template <typename TFeature>
	TFeature* FindInteractionFeature(const ALxPlayerController& PlayerController, AActor* InteractionOwner,
		int32 RuntimeNodeIndex, int32 InteractionTreeRevision,
		ELxInteractionActionType ExpectedType)
	{
		if (!InteractionOwner)
		{
			return nullptr;
		}
		const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(PlayerController.GetPawn());
		ULxPlayerInteractionModule* PlayerInteractionModule = PlayerCharacter
			? PlayerCharacter->GetPlayerInteractionComponent()
			: nullptr;
		ULxInteractableComponent* InteractableComponent = InteractionOwner->FindComponentByClass<ULxInteractableComponent>();
		if (!PlayerInteractionModule || !InteractableComponent
			|| InteractableComponent->GetInteractionTreeRevision() != InteractionTreeRevision
			|| !PlayerInteractionModule->IsInteractableComponentInRange(InteractableComponent))
		{
			return nullptr;
		}

		ULxInteractionNode* InteractionNode = InteractableComponent->FindInteractionNodeByRuntimeIndex(RuntimeNodeIndex);
		if (!InteractionNode || InteractionNode->GetInteractionActionType() != ExpectedType
			|| !InteractionNode->CanProcessActiveInteractionRequest(PlayerInteractionModule))
		{
			return nullptr;
		}

		return Cast<TFeature>(InteractionNode->GetInteractionFeature());
	}
}

ALxPlayerController::ALxPlayerController()
{
	m_pInputComponent = CreateDefaultSubobject<ULxInputComponent>(TEXT("外部输入管理组件"));
	m_pSystemOperateComponent = CreateDefaultSubobject<ULxPlayerSystemOperateComponent>(TEXT("系统操作组件"));
	m_pChatComponent = CreateDefaultSubobject<ULxPlayerChatComponent>(TEXT("玩家聊天组件"));

}


void ALxPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
	{
		LocalPlayerSubsystem->SetPlayerControllerQuote(this);
		LocalPlayerSubsystem->SetControlledCharacter(m_pCurrentCharacter);
	}
	if (m_pSystemOperateComponent)
	{
		m_pSystemOperateComponent->BaseComponentInitialize();
	}
	if (m_pInputComponent)
	{
		
		m_pInputComponent->BaseComponentInitialize();
	}
}

void ALxPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (m_pInputComponent)
	{
		m_pInputComponent->BaseComponentInitialize();
	}
}

void ALxPlayerController::CreatePlayerCharacter()
{
	if (GetNetMode() == NM_Standalone)
	{
		SpawnAndPossessPlayerCharacter();
		return;
	}

	CreateServerPlayerCharacter();
}

void ALxPlayerController::ServerMoveItemBetweenBackpackAndWarehouse_Implementation(AActor* WarehouseOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 SourceSlotIndex, int32 TargetSlotIndex, bool bMoveToWarehouse)
{
	ULxWarehouseInteractionComponent* WarehouseComponent = FindInteractionFeature<ULxWarehouseInteractionComponent>(
		*this, WarehouseOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::Warehouse);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (WarehouseComponent == nullptr || BackpackComponent == nullptr)
	{
		return;
	}

	if (bMoveToWarehouse)
	{
		WarehouseComponent->MoveBackpackSlotToWarehouse(BackpackComponent, SourceSlotIndex, TargetSlotIndex);
		return;
	}

	WarehouseComponent->MoveWarehouseSlotToBackpack(BackpackComponent, SourceSlotIndex, TargetSlotIndex);
}

void ALxPlayerController::ServerMoveBackpackSlot_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (BackpackComponent)
	{
		BackpackComponent->MoveBackpackSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveWarehouseSlot_Implementation(AActor* WarehouseOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (ULxWarehouseInteractionComponent* WarehouseComponent = FindInteractionFeature<ULxWarehouseInteractionComponent>(
		*this, WarehouseOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::Warehouse))
	{
		WarehouseComponent->MoveWarehouseSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveTreasureChestSlotToBackpack_Implementation(AActor* TreasureChestOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 TreasureChestSlotIndex, int32 BackpackSlotIndex)
{
	ULxTreasureChestInteractionComponent* TreasureChestComponent = FindInteractionFeature<ULxTreasureChestInteractionComponent>(
		*this, TreasureChestOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TreasureChest);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TreasureChestComponent && BackpackComponent)
	{
		TreasureChestComponent->MoveTreasureChestSlotToBackpack(BackpackComponent, TreasureChestSlotIndex, BackpackSlotIndex);
	}
}

void ALxPlayerController::ServerBuyTradeSlot_Implementation(AActor* TradeOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision,
	int32 TradeSlotIndex)
{
	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		*this, TradeOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (TradeComponent && DataTransferComponent)
	{
		TradeComponent->BuyTradeSlot(TradeComponent->GetTradeSlotAt(TradeSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerBuyTradeSlotToBackpackSlot_Implementation(AActor* TradeOwner,
	int32 RuntimeNodeIndex, int32 InteractionTreeRevision, int32 TradeSlotIndex, int32 BackpackSlotIndex)
{
	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		*this, TradeOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->BuyTradeSlotToBackpackSlot(
			TradeComponent->GetTradeSlotAt(TradeSlotIndex),
			BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex),
			DataTransferComponent);
	}
}

void ALxPlayerController::ServerSellBackpackSlot_Implementation(AActor* TradeOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision,
	int32 BackpackSlotIndex)
{
	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		*this, TradeOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->SellBackpackSlot(BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerExecuteItemTransfer_Implementation(AActor* ItemTransferOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision)
{
	ULxItemTransferInteractionComponent* ItemTransferComponent = FindInteractionFeature<ULxItemTransferInteractionComponent>(
		*this, ItemTransferOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::ItemTransfer);
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetPawn());
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter ? PlayerCharacter->GetPlayerInteractionComponent() : nullptr;
	if (ItemTransferComponent && PlayerInteractionComponent)
	{
		ItemTransferComponent->ExecuteInteraction(PlayerInteractionComponent);
	}
}

void ALxPlayerController::ServerTriggerMechanism_Implementation(AActor* MechanismOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision)
{
	ULxTriggerMechanismInteractionComponent* TriggerMechanismComponent = FindInteractionFeature<ULxTriggerMechanismInteractionComponent>(
		*this, MechanismOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::TriggerMechanism);
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetPawn());
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter ? PlayerCharacter->GetPlayerInteractionComponent() : nullptr;
	if (TriggerMechanismComponent && PlayerInteractionComponent)
	{
		TriggerMechanismComponent->TriggerMechanism(PlayerInteractionComponent);
	}
}

void ALxPlayerController::ServerExecuteQuestInteraction_Implementation(
	AActor* QuestOwner, int32 RuntimeNodeIndex, int32 InteractionTreeRevision)
{
	ULxQuestInteractionComponent* QuestInteractionComponent =
		FindInteractionFeature<ULxQuestInteractionComponent>(
			*this, QuestOwner, RuntimeNodeIndex, InteractionTreeRevision, ELxInteractionActionType::Quest);
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetPawn());
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter
		? PlayerCharacter->GetPlayerInteractionComponent()
		: nullptr;
	if (QuestInteractionComponent && PlayerInteractionComponent)
	{
		QuestInteractionComponent->ExecuteInteraction(PlayerInteractionComponent);
	}
}

void ALxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SyncControlledCharacter(InPawn);
}

void ALxPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	SyncControlledCharacter(InPawn);
}

void ALxPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	m_pCurrentCharacter = nullptr;
	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
	{
		LocalPlayerSubsystem->SetControlledCharacter(nullptr);
	}
}

void ALxPlayerController::SyncControlledCharacter(APawn* InPawn)
{
	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
	if (m_pCurrentCharacter)
	{
		m_pCurrentCharacter->InitialCharacterInformation();
	}
	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
	{
		LocalPlayerSubsystem->SetControlledCharacter(m_pCurrentCharacter);
	}
}

void ALxPlayerController::SpawnAndPossessPlayerCharacter()
{
	if (ALxARPGGameMode* GameMode = GetWorld() ? Cast<ALxARPGGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (APawn* NewPawn = GameMode->SpawnPlayerCharacter(this))
		{
			Possess(NewPawn);
		}
	}
}

void ALxPlayerController::ShowCursorFun()
{
	// 显示鼠标
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ALxPlayerController::HideCursorFun()
{
	// 隐藏鼠标
	FInputModeGameOnly Mode;
	SetInputMode(Mode);

	bShowMouseCursor = false;
}

void ALxPlayerController::CreateServerPlayerCharacter_Implementation()
{
	SpawnAndPossessPlayerCharacter();
}
