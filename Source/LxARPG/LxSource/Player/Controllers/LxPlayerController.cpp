// Fill out your copyright notice in the Description page of Project Settings.


#include "LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxItemTransferInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxPlayerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Chat/Logic/LxPlayerChatComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/SystemOperate/LxPlayerSystemOperateComponent.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

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

void ALxPlayerController::ServerMoveItemBetweenBackpackAndWarehouse_Implementation(AActor* WarehouseOwner, int32 SourceSlotIndex, int32 TargetSlotIndex, bool bMoveToWarehouse)
{
	if (WarehouseOwner == nullptr)
	{
		return;
	}

	ULxWarehouseInteractionComponent* WarehouseComponent = WarehouseOwner->FindComponentByClass<ULxWarehouseInteractionComponent>();
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackComponent* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
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
	ULxCharacterBackpackComponent* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (BackpackComponent)
	{
		BackpackComponent->MoveBackpackSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveWarehouseSlot_Implementation(AActor* WarehouseOwner, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (WarehouseOwner == nullptr)
	{
		return;
	}

	if (ULxWarehouseInteractionComponent* WarehouseComponent = WarehouseOwner->FindComponentByClass<ULxWarehouseInteractionComponent>())
	{
		WarehouseComponent->MoveWarehouseSlot(SourceSlotIndex, TargetSlotIndex);
	}
}

void ALxPlayerController::ServerMoveTreasureChestSlotToBackpack_Implementation(AActor* TreasureChestOwner, int32 TreasureChestSlotIndex, int32 BackpackSlotIndex)
{
	if (TreasureChestOwner == nullptr)
	{
		return;
	}

	ULxTreasureChestInteractionComponent* TreasureChestComponent = TreasureChestOwner->FindComponentByClass<ULxTreasureChestInteractionComponent>();
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterBackpackComponent* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TreasureChestComponent && BackpackComponent)
	{
		TreasureChestComponent->MoveTreasureChestSlotToBackpack(BackpackComponent, TreasureChestSlotIndex, BackpackSlotIndex);
	}
}

void ALxPlayerController::ServerBuyTradeSlot_Implementation(AActor* TradeOwner, int32 TradeSlotIndex)
{
	if (TradeOwner == nullptr)
	{
		return;
	}

	ULxTradeContainerInteractionComponent* TradeComponent = TradeOwner->FindComponentByClass<ULxTradeContainerInteractionComponent>();
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (TradeComponent && DataTransferComponent)
	{
		TradeComponent->BuyTradeSlot(TradeComponent->GetTradeSlotAt(TradeSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerBuyTradeSlotToBackpackSlot_Implementation(AActor* TradeOwner, int32 TradeSlotIndex, int32 BackpackSlotIndex)
{
	if (TradeOwner == nullptr)
	{
		return;
	}

	ULxTradeContainerInteractionComponent* TradeComponent = TradeOwner->FindComponentByClass<ULxTradeContainerInteractionComponent>();
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackComponent* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->BuyTradeSlotToBackpackSlot(
			TradeComponent->GetTradeSlotAt(TradeSlotIndex),
			BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex),
			DataTransferComponent);
	}
}

void ALxPlayerController::ServerSellBackpackSlot_Implementation(AActor* TradeOwner, int32 BackpackSlotIndex)
{
	if (TradeOwner == nullptr)
	{
		return;
	}

	ULxTradeContainerInteractionComponent* TradeComponent = TradeOwner->FindComponentByClass<ULxTradeContainerInteractionComponent>();
	const ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxCharacterDataTransferComponent* DataTransferComponent = CurrentCharacter ? CurrentCharacter->GetCharacterDataTransferComponent() : nullptr;
	ULxCharacterBackpackComponent* BackpackComponent = CurrentCharacter ? CurrentCharacter->GetCharacterBackpackComponent() : nullptr;
	if (TradeComponent && DataTransferComponent && BackpackComponent)
	{
		TradeComponent->SellBackpackSlot(BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex), DataTransferComponent);
	}
}

void ALxPlayerController::ServerExecuteItemTransfer_Implementation(AActor* ItemTransferOwner)
{
	if (ItemTransferOwner == nullptr)
	{
		return;
	}

	ULxItemTransferInteractionComponent* ItemTransferComponent = ItemTransferOwner->FindComponentByClass<ULxItemTransferInteractionComponent>();
	ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxPlayerInteractionComponent* PlayerInteractionComponent = CurrentCharacter ? CurrentCharacter->FindComponentByClass<ULxPlayerInteractionComponent>() : nullptr;
	if (ItemTransferComponent && PlayerInteractionComponent)
	{
		ItemTransferComponent->ExecuteInteraction(PlayerInteractionComponent);
	}
}

void ALxPlayerController::ServerTriggerMechanism_Implementation(AActor* MechanismOwner)
{
	if (MechanismOwner == nullptr)
	{
		return;
	}

	ULxTriggerMechanismInteractionComponent* TriggerMechanismComponent = MechanismOwner->FindComponentByClass<ULxTriggerMechanismInteractionComponent>();
	ALxBaseCharacter* CurrentCharacter = Cast<ALxBaseCharacter>(GetPawn());
	ULxPlayerInteractionComponent* PlayerInteractionComponent = CurrentCharacter ? CurrentCharacter->FindComponentByClass<ULxPlayerInteractionComponent>() : nullptr;
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
