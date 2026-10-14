#include "LxTradeContainerWidget.h"

#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemOperationUIRouter.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

bool ULxTradeContainerWidget::GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const
{
	if (!IsVisible() || !GetIsEnabled() || !TradeContainerComponent || !TradeContainerComponent->GetOwner()) return false;
	OutContainer = FLxItemContainerAddress();
	OutContainer.Kind = ELxItemContainerKind::Trade;
	OutContainer.Owner = TradeContainerComponent->GetOwner();
	OutContainer.RuntimeNodeIndex = TradeContainerComponent->GetRuntimeNodeIndex();
	OutContainer.InteractionTreeRevision = TradeContainerComponent->GetInteractionTreeRevision();
	return true;
}

ULxItemTransferTicket* ULxTradeContainerWidget::PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason)
{
	FLxItemContainerAddress Address;
	if (!GetQuickItemContainer_Implementation(Address) || !SourceSlot
		|| TradeContainerComponent->GetTradeSlotAt(SourceSlot->GetSlotIndex()) != SourceSlot)
	{
		OutReason = NSLOCTEXT("LxTradeOperations", "TradeWindowSourceInvalid", "交易窗口或商品槽位已经失效");
		return nullptr;
	}
	return TradeContainerComponent->BeginSafeExtractTradeItem(SourceSlot->GetSlotIndex(), OutReason);
}

bool ULxTradeContainerWidget::CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const
{
	FLxItemContainerAddress Address;
	const ALxBaseCharacter* Player = GetOwningPlayer() ? Cast<ALxBaseCharacter>(GetOwningPlayer()->GetPawn()) : nullptr;
	ULxCharacterDataTransferComponent* DataTransfer = Player ? Player->GetCharacterDataTransferComponent() : nullptr;
	if (!GetQuickItemContainer_Implementation(Address) || !Ticket || Ticket->bCompleted || !Ticket->SourceSlot
		|| Ticket->SourceSlot->GetSlotType() != ELxItemSlotType::Backpack || Ticket->SourceSlot->GetItem() != Ticket->Item
		|| !LxSafeItemOperations::MatchesExpectedRecord(Ticket->SourceSlot, Ticket->ExpectedItem)
		|| !TradeContainerComponent->CanSellBackpackSlot(Ticket->SourceSlot, DataTransfer))
	{
		OutReason = NSLOCTEXT("LxTradeOperations", "CannotQuickSell", "只能出售当前背包中的物品；物品已变化、没有收购价或金币空间不足");
		return false;
	}
	return true;
}

bool ULxTradeContainerWidget::ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket,
	const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason)
{
	if (Request.Source.Kind != ELxItemContainerKind::Backpack || !CanReceiveQuickItemTransfer_Implementation(Ticket, OutReason)) return false;
	FLxItemOperationRequest Received = Request;
	if (!GetQuickItemContainer_Implementation(Received.Target)) return false;
	ULxUIManager* Layout = GetOwningUIManager();
	return Layout && Layout->GetItemOperationRouter()->SubmitOperation(Received, Ticket, SourceWindow, OutReason);
}

void ULxTradeContainerWidget::NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result)
{
	LastItemOperationResult = Result;
	OnItemOperationCompleted(Result);
}

void ULxTradeContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideTradeInteraction();
}

void ULxTradeContainerWidget::NativeDestruct()
{
	HideTradeInteraction();
	UnbindTradeContainerComponent();
	UnbindPlayerInteractionComponent();
	Super::NativeDestruct();
}

void ULxTradeContainerWidget::SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent)
{
	if (PlayerInteractionComponent == InPlayerInteractionComponent)
	{
		return;
	}

	UnbindPlayerInteractionComponent();
	PlayerInteractionComponent = InPlayerInteractionComponent;
	BindPlayerInteractionComponent();

	if (!PlayerInteractionComponent)
	{
		SetTradeContainerComponent(nullptr);
	}
}

void ULxTradeContainerWidget::SetTradeContainerComponent(ULxTradeContainerInteractionComponent* InTradeContainerComponent)
{
	if (TradeContainerComponent == InTradeContainerComponent)
	{
		RefreshTradeItemList();
		if (TradeContainerComponent)
		{
			ShowTradeInteraction();
		}
		else
		{
			HideTradeInteraction();
		}
		return;
	}

	if (TradeContainerComponent)
	{
		TradeContainerComponent->SetTradeContainerState(ELxInteractionDataState::Interactable);
	}

	UnbindTradeContainerComponent();
	TradeContainerComponent = InTradeContainerComponent;
	BindTradeContainerComponent();
	RefreshTradeItemList();
	if (TradeContainerComponent)
	{
		ShowTradeInteraction();
		OnTradeContainerOpened(TradeContainerComponent);
	}
	else
	{
		HideTradeInteraction();
		OnTradeContainerClosed();
	}
}

void ULxTradeContainerWidget::CloseTradeInteraction()
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->CancelInteraction();
		return;
	}

	SetTradeContainerComponent(nullptr);
}

TArray<UObject*> ULxTradeContainerWidget::GetTradeItemUIDataList()
{
	TArray<UObject*> ItemUIDataList;
	for (ULxItemSlotData* ItemSlot : TradeItemSlotList)
	{
		ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(this);
		ItemUIData->m_pSlotData = ItemSlot;
		ItemUIData->ItemOwnerWindow = this;
		ItemUIDataList.Add(ItemUIData);
	}
	return ItemUIDataList;
}

void ULxTradeContainerWidget::ShowTradeInteraction()
{
	SetVisibility(ESlateVisibility::Visible);
	SetMouseCursorVisible(true);
}

void ULxTradeContainerWidget::HideTradeInteraction()
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetMouseCursorVisible(false);
}

void ULxTradeContainerWidget::SetMouseCursorVisible(bool bInVisible)
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = ULxLocalPlayerSubsystem::GetFromLocalPlayer(PlayerController->GetLocalPlayer()))
	{
		if (ULxUIManager* UIManager = LocalPlayerSubsystem->GetUIManager())
		{
			UIManager->RefreshCursorState();
			return;
		}
	}

	if (bInVisible)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
		return;
	}

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = false;
}

void ULxTradeContainerWidget::BindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveAll(this);
	PlayerInteractionComponent->OnInteractionOptionActivated.AddUObject(this, &ULxTradeContainerWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveAll(this);
	PlayerInteractionComponent->OnInteractionCancelled.AddUObject(this, &ULxTradeContainerWidget::HandleInteractionCancelled);
}

void ULxTradeContainerWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveAll(this);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveAll(this);
	PlayerInteractionComponent = nullptr;
}

void ULxTradeContainerWidget::BindTradeContainerComponent()
{
	if (!TradeContainerComponent)
	{
		return;
	}

	TradeContainerComponent->OnTradeSlotListChanged.RemoveAll(this);
	TradeContainerComponent->OnTradeSlotListChanged.AddUObject(this, &ULxTradeContainerWidget::HandleTradeSlotListChanged);
}

void ULxTradeContainerWidget::UnbindTradeContainerComponent()
{
	if (!TradeContainerComponent)
	{
		return;
	}

	TradeContainerComponent->OnTradeSlotListChanged.RemoveAll(this);
	TradeContainerComponent = nullptr;
	TradeItemSlotList.Reset();
}

void ULxTradeContainerWidget::RefreshTradeItemList()
{
	TradeItemSlotList.Reset();

	if (TradeContainerComponent)
	{
		TArray<ULxItemSlotData*> Slots;
		TradeContainerComponent->GetTradeItemSlotList(Slots);
		HandleTradeSlotListChanged(Slots);
		return;
	}

	OnTradeItemListUpdated(GetTradeItemUIDataList());
}

void ULxTradeContainerWidget::HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	if (InteractionType != ELxInteractionActionType::TradeContainer || !Option.InteractionNode)
	{
		return;
	}

	SetTradeContainerComponent(Cast<ULxTradeContainerInteractionComponent>(Option.InteractionNode->GetInteractionFeature()));
}

void ULxTradeContainerWidget::HandleInteractionCancelled()
{
	SetTradeContainerComponent(nullptr);
}

void ULxTradeContainerWidget::HandleTradeSlotListChanged(const TArray<ULxItemSlotData*>& TradeSlots)
{
	TradeItemSlotList.Reset();
	for (ULxItemSlotData* SlotData : TradeSlots)
	{
		TradeItemSlotList.Add(SlotData);
	}

	OnTradeItemListUpdated(GetTradeItemUIDataList());
}
