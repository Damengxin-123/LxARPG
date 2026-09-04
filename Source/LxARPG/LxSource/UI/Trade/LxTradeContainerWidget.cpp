#include "LxTradeContainerWidget.h"

#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

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

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxTradeContainerWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionOptionActivated.AddDynamic(this, &ULxTradeContainerWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxTradeContainerWidget::HandleInteractionCancelled);
	PlayerInteractionComponent->OnInteractionCancelled.AddDynamic(this, &ULxTradeContainerWidget::HandleInteractionCancelled);
}

void ULxTradeContainerWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxTradeContainerWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxTradeContainerWidget::HandleInteractionCancelled);
	PlayerInteractionComponent = nullptr;
}

void ULxTradeContainerWidget::BindTradeContainerComponent()
{
	if (!TradeContainerComponent)
	{
		return;
	}

	TradeContainerComponent->OnTradeSlotListChanged.RemoveDynamic(this, &ULxTradeContainerWidget::HandleTradeSlotListChanged);
	TradeContainerComponent->OnTradeSlotListChanged.AddDynamic(this, &ULxTradeContainerWidget::HandleTradeSlotListChanged);
}

void ULxTradeContainerWidget::UnbindTradeContainerComponent()
{
	if (!TradeContainerComponent)
	{
		return;
	}

	TradeContainerComponent->OnTradeSlotListChanged.RemoveDynamic(this, &ULxTradeContainerWidget::HandleTradeSlotListChanged);
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
