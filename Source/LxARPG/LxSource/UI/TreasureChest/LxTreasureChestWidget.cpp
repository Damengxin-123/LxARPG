#include "LxTreasureChestWidget.h"

#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

void ULxTreasureChestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideTreasureChestInteraction();
}

void ULxTreasureChestWidget::NativeDestruct()
{
	HideTreasureChestInteraction();
	UnbindTreasureChestComponent();
	UnbindPlayerInteractionComponent();
	Super::NativeDestruct();
}

void ULxTreasureChestWidget::SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent)
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
		SetTreasureChestComponent(nullptr);
	}
}

void ULxTreasureChestWidget::SetTreasureChestComponent(ULxTreasureChestInteractionComponent* InTreasureChestComponent)
{
	if (TreasureChestComponent == InTreasureChestComponent)
	{
		RefreshTreasureChestItemList();
		if (TreasureChestComponent)
		{
			ShowTreasureChestInteraction();
		}
		else
		{
			HideTreasureChestInteraction();
		}
		return;
	}

	if (TreasureChestComponent && TreasureChestComponent->GetInteractionState() != ELxInteractionDataState::Finished)
	{
		TreasureChestComponent->SetTreasureChestState(ELxInteractionDataState::Interactable);
	}

	UnbindTreasureChestComponent();
	TreasureChestComponent = InTreasureChestComponent;
	BindTreasureChestComponent();
	RefreshTreasureChestItemList();

	if (TreasureChestComponent)
	{
		ShowTreasureChestInteraction();
		OnTreasureChestOpened(TreasureChestComponent);
	}
	else
	{
		HideTreasureChestInteraction();
		OnTreasureChestClosed();
	}
}

void ULxTreasureChestWidget::CloseTreasureChestInteraction()
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->CancelInteraction();
		return;
	}

	SetTreasureChestComponent(nullptr);
}

TArray<UObject*> ULxTreasureChestWidget::GetTreasureChestItemUIDataList()
{
	TArray<UObject*> ItemUIDataList;
	for (ULxItemSlotData* ItemSlot : TreasureChestItemSlotList)
	{
		ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(this);
		ItemUIData->m_pSlotData = ItemSlot;
		ItemUIDataList.Add(ItemUIData);
	}
	return ItemUIDataList;
}

void ULxTreasureChestWidget::ShowTreasureChestInteraction()
{
	SetVisibility(ESlateVisibility::Visible);
	SetMouseCursorVisible(true);
}

void ULxTreasureChestWidget::HideTreasureChestInteraction()
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetMouseCursorVisible(false);
}

void ULxTreasureChestWidget::SetMouseCursorVisible(bool bInVisible)
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

void ULxTreasureChestWidget::BindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxTreasureChestWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionOptionActivated.AddDynamic(this, &ULxTreasureChestWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxTreasureChestWidget::HandleInteractionCancelled);
	PlayerInteractionComponent->OnInteractionCancelled.AddDynamic(this, &ULxTreasureChestWidget::HandleInteractionCancelled);
}

void ULxTreasureChestWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxTreasureChestWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxTreasureChestWidget::HandleInteractionCancelled);
	PlayerInteractionComponent = nullptr;
}

void ULxTreasureChestWidget::BindTreasureChestComponent()
{
	if (!TreasureChestComponent)
	{
		return;
	}

	TreasureChestComponent->OnTreasureChestSlotListChanged.RemoveDynamic(this, &ULxTreasureChestWidget::HandleTreasureChestSlotListChanged);
	TreasureChestComponent->OnTreasureChestSlotListChanged.AddDynamic(this, &ULxTreasureChestWidget::HandleTreasureChestSlotListChanged);
	TreasureChestComponent->OnItemAcquireCompleted.RemoveDynamic(this, &ULxTreasureChestWidget::HandleTreasureChestCompleted);
	TreasureChestComponent->OnItemAcquireCompleted.AddDynamic(this, &ULxTreasureChestWidget::HandleTreasureChestCompleted);
}

void ULxTreasureChestWidget::UnbindTreasureChestComponent()
{
	if (!TreasureChestComponent)
	{
		return;
	}

	TreasureChestComponent->OnTreasureChestSlotListChanged.RemoveDynamic(this, &ULxTreasureChestWidget::HandleTreasureChestSlotListChanged);
	TreasureChestComponent->OnItemAcquireCompleted.RemoveDynamic(this, &ULxTreasureChestWidget::HandleTreasureChestCompleted);
	TreasureChestComponent = nullptr;
	TreasureChestItemSlotList.Reset();
}

void ULxTreasureChestWidget::RefreshTreasureChestItemList()
{
	TreasureChestItemSlotList.Reset();

	if (TreasureChestComponent)
	{
		TArray<ULxItemSlotData*> Slots;
		TreasureChestComponent->GetTreasureChestItemSlotList(Slots);
		HandleTreasureChestSlotListChanged(Slots);
		return;
	}

	OnTreasureChestItemListUpdated(GetTreasureChestItemUIDataList());
}

void ULxTreasureChestWidget::HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	if (InteractionType != ELxInteractionActionType::TreasureChest || !Option.InteractionNode)
	{
		return;
	}

	SetTreasureChestComponent(Cast<ULxTreasureChestInteractionComponent>(Option.InteractionNode->GetInteractionFeature()));
}

void ULxTreasureChestWidget::HandleInteractionCancelled()
{
	SetTreasureChestComponent(nullptr);
}

void ULxTreasureChestWidget::HandleTreasureChestSlotListChanged(const TArray<ULxItemSlotData*>& TreasureChestSlots)
{
	TreasureChestItemSlotList.Reset();
	for (ULxItemSlotData* SlotData : TreasureChestSlots)
	{
		TreasureChestItemSlotList.Add(SlotData);
	}

	OnTreasureChestItemListUpdated(GetTreasureChestItemUIDataList());
}

void ULxTreasureChestWidget::HandleTreasureChestCompleted()
{
	OnTreasureChestCompleted();
}
