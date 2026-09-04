// Fill out your copyright notice in the Description page of Project Settings.


#include "LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxItemTransferInteractionComponent.h"
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
	/** 根据可交互对象和运行时节点序号获取指定类型的功能模块。 */
	template <typename TFeature>
	TFeature* FindInteractionFeature(ALxPlayerController* PlayerController, AActor* InteractionOwner,
		int32 RuntimeNodeIndex,
		ELxInteractionActionType ExpectedType)
	{
		const ALxPlayerCharacter* PlayerCharacter = PlayerController
			? Cast<ALxPlayerCharacter>(PlayerController->GetPawn())
			: nullptr;
		ULxPlayerInteractionModule* PlayerInteractionModule = PlayerCharacter
			? PlayerCharacter->GetPlayerInteractionComponent()
			: nullptr;
		ULxInteractableComponent* InteractableComponent = InteractionOwner
			? InteractionOwner->FindComponentByClass<ULxInteractableComponent>()
			: nullptr;
		if (!PlayerInteractionModule
			|| !PlayerInteractionModule->IsInteractableComponentInRange(InteractableComponent))
		{
			return nullptr;
		}

		ULxInteractionNode* InteractionNode = InteractableComponent
			? InteractableComponent->FindInteractionNodeByRuntimeIndex(RuntimeNodeIndex)
			: nullptr;
		if (!InteractionNode || InteractionNode->GetInteractionActionType() != ExpectedType
			|| !InteractionNode->IsNodeInteractable(PlayerInteractionModule))
		{
			return nullptr;
		}

		return Cast<TFeature>(InteractionNode->GetActionComponent());
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
	if (m_pInputComponent)
	{
		if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
		{
			LocalPlayerSubsystem->SetInputComponentQuote(m_pInputComponent);
		}
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
		CreateLocalPlayerCharacter();
		return;
	}

	CreateServerPlayerCharacter();
}

void ALxPlayerController::ServerMoveItemBetweenBackpackAndWarehouse_Implementation(AActor* WarehouseOwner,
	int32 RuntimeNodeIndex, int32 SourceSlotIndex, int32 TargetSlotIndex, bool bMoveToWarehouse)
{
	if (WarehouseOwner == nullptr)
	{
		return;
	}

	ULxWarehouseInteractionComponent* WarehouseComponent = FindInteractionFeature<ULxWarehouseInteractionComponent>(
		this, WarehouseOwner, RuntimeNodeIndex, ELxInteractionActionType::Warehouse);
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
	int32 RuntimeNodeIndex, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (WarehouseOwner == nullptr)
	{
		return;
	}

	if (ULxWarehouseInteractionComponent* WarehouseComponent = FindInteractionFeature<ULxWarehouseInteractionComponent>(
		this, WarehouseOwner, RuntimeNodeIndex, ELxInteractionActionType::Warehouse))
	{
		WarehouseComponent->MoveWarehouseSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveTreasureChestSlotToBackpack_Implementation(AActor* TreasureChestOwner,
	int32 RuntimeNodeIndex, int32 TreasureChestSlotIndex, int32 BackpackSlotIndex)
{
	if (TreasureChestOwner == nullptr)
	{
		return;
	}

	ULxTreasureChestInteractionComponent* TreasureChestComponent = FindInteractionFeature<ULxTreasureChestInteractionComponent>(
		this, TreasureChestOwner, RuntimeNodeIndex, ELxInteractionActionType::TreasureChest);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TreasureChestComponent && BackpackComponent)
	{
		TreasureChestComponent->MoveTreasureChestSlotToBackpack(BackpackComponent, TreasureChestSlotIndex, BackpackSlotIndex);
	}
}

void ALxPlayerController::ServerBuyTradeSlot_Implementation(AActor* TradeOwner, int32 RuntimeNodeIndex,
	int32 TradeSlotIndex)
{
	if (TradeOwner == nullptr)
	{
		return;
	}

	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		this, TradeOwner, RuntimeNodeIndex, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (TradeComponent && DataTransferComponent)
	{
		TradeComponent->BuyTradeSlot(TradeComponent->GetTradeSlotAt(TradeSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerBuyTradeSlotToBackpackSlot_Implementation(AActor* TradeOwner,
	int32 RuntimeNodeIndex, int32 TradeSlotIndex, int32 BackpackSlotIndex)
{
	if (TradeOwner == nullptr)
	{
		return;
	}

	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		this, TradeOwner, RuntimeNodeIndex, ELxInteractionActionType::TradeContainer);
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

void ALxPlayerController::ServerSellBackpackSlot_Implementation(AActor* TradeOwner, int32 RuntimeNodeIndex,
	int32 BackpackSlotIndex)
{
	if (TradeOwner == nullptr)
	{
		return;
	}

	ULxTradeContainerInteractionComponent* TradeComponent = FindInteractionFeature<ULxTradeContainerInteractionComponent>(
		this, TradeOwner, RuntimeNodeIndex, ELxInteractionActionType::TradeContainer);
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackModule* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->SellBackpackSlot(BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerExecuteItemTransfer_Implementation(AActor* ItemTransferOwner, int32 RuntimeNodeIndex)
{
	if (ItemTransferOwner == nullptr)
	{
		return;
	}

	ULxItemTransferInteractionComponent* ItemTransferComponent = FindInteractionFeature<ULxItemTransferInteractionComponent>(
		this, ItemTransferOwner, RuntimeNodeIndex, ELxInteractionActionType::ItemTransfer);
	ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(CurrentCharacter);
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter ? PlayerCharacter->GetPlayerInteractionComponent() : nullptr;
	if (ItemTransferComponent && PlayerInteractionComponent)
	{
		ItemTransferComponent->ExecuteInteraction(PlayerInteractionComponent);
	}
}

void ALxPlayerController::ServerTriggerMechanism_Implementation(AActor* MechanismOwner, int32 RuntimeNodeIndex)
{
	if (MechanismOwner == nullptr)
	{
		return;
	}

	ULxTriggerMechanismInteractionComponent* TriggerMechanismComponent = FindInteractionFeature<ULxTriggerMechanismInteractionComponent>(
		this, MechanismOwner, RuntimeNodeIndex, ELxInteractionActionType::TriggerMechanism);
	ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	const ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(CurrentCharacter);
	ULxPlayerInteractionModule* PlayerInteractionComponent = PlayerCharacter ? PlayerCharacter->GetPlayerInteractionComponent() : nullptr;
	if (TriggerMechanismComponent && PlayerInteractionComponent)
	{
		TriggerMechanismComponent->TriggerMechanism(PlayerInteractionComponent);
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

void ALxPlayerController::CreateLocalPlayerCharacter()
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
	if (ALxARPGGameMode* GameMode = GetWorld() ? Cast<ALxARPGGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (APawn* NewPawn = GameMode->SpawnPlayerCharacter(this))
		{
			Possess(NewPawn);
		}
	}
}
