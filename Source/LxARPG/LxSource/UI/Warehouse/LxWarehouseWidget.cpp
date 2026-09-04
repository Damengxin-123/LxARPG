#include "LxWarehouseWidget.h"

#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "GameFramework/PlayerController.h"

void ULxWarehouseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideWarehouseInteraction();
}

void ULxWarehouseWidget::NativeDestruct()
{
	HideWarehouseInteraction();
	UnbindWarehouseComponent();
	UnbindPlayerInteractionComponent();
	Super::NativeDestruct();
}

void ULxWarehouseWidget::SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent)
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
		SetWarehouseComponent(nullptr);
	}
}

void ULxWarehouseWidget::SetWarehouseComponent(ULxWarehouseInteractionComponent* InWarehouseComponent)
{
	if (WarehouseComponent == InWarehouseComponent)
	{
		RefreshWarehouseItemList();
		if (WarehouseComponent)
		{
			ShowWarehouseInteraction();
		}
		else
		{
			HideWarehouseInteraction();
		}
		return;
	}

	if (WarehouseComponent)
	{
		WarehouseComponent->SetWarehouseState(ELxInteractionDataState::Interactable);
	}

	UnbindWarehouseComponent();
	WarehouseComponent = InWarehouseComponent;
	BindWarehouseComponent();
	RefreshWarehouseItemList();
	if (WarehouseComponent)
	{
		ShowWarehouseInteraction();
		OnWarehouseOpened(WarehouseComponent);
	}
	else
	{
		HideWarehouseInteraction();
		OnWarehouseClosed();
	}
}

void ULxWarehouseWidget::CloseWarehouseInteraction()
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->CancelInteraction();
		return;
	}

	SetWarehouseComponent(nullptr);
}

TArray<UObject*> ULxWarehouseWidget::GetWarehouseItemUIDataList()
{
	TArray<UObject*> ItemUIDataList;
	for (ULxItemSlotData* ItemSlot : WarehouseItemSlotList)
	{
		ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(this);
		ItemUIData->m_pSlotData = ItemSlot;
		ItemUIDataList.Add(ItemUIData);
	}
	return ItemUIDataList;
}

void ULxWarehouseWidget::ShowWarehouseInteraction()
{
	SetVisibility(ESlateVisibility::Visible);
	SetMouseCursorVisible(true);
}

void ULxWarehouseWidget::HideWarehouseInteraction()
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetMouseCursorVisible(false);
}

void ULxWarehouseWidget::SetMouseCursorVisible(bool bInVisible)
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

void ULxWarehouseWidget::BindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxWarehouseWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionOptionActivated.AddDynamic(this, &ULxWarehouseWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxWarehouseWidget::HandleInteractionCancelled);
	PlayerInteractionComponent->OnInteractionCancelled.AddDynamic(this, &ULxWarehouseWidget::HandleInteractionCancelled);
}

void ULxWarehouseWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxWarehouseWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxWarehouseWidget::HandleInteractionCancelled);
	PlayerInteractionComponent = nullptr;
}

void ULxWarehouseWidget::BindWarehouseComponent()
{
	if (!WarehouseComponent)
	{
		return;
	}

	WarehouseComponent->OnWarehouseSlotListChanged.RemoveDynamic(this, &ULxWarehouseWidget::HandleWarehouseSlotListChanged);
	WarehouseComponent->OnWarehouseSlotListChanged.AddDynamic(this, &ULxWarehouseWidget::HandleWarehouseSlotListChanged);
}

void ULxWarehouseWidget::UnbindWarehouseComponent()
{
	if (!WarehouseComponent)
	{
		return;
	}

	WarehouseComponent->OnWarehouseSlotListChanged.RemoveDynamic(this, &ULxWarehouseWidget::HandleWarehouseSlotListChanged);
	WarehouseComponent = nullptr;
	WarehouseItemSlotList.Reset();
}

void ULxWarehouseWidget::RefreshWarehouseItemList()
{
	WarehouseItemSlotList.Reset();

	if (WarehouseComponent)
	{
		TArray<ULxItemSlotData*> Slots;
		WarehouseComponent->GetWarehouseItemSlotList(Slots);
		HandleWarehouseSlotListChanged(Slots);
		return;
	}

	OnWarehouseItemListUpdated(GetWarehouseItemUIDataList());
}

void ULxWarehouseWidget::HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	if (InteractionType != ELxInteractionActionType::Warehouse || !Option.InteractionNode)
	{
		return;
	}

	// 仓库类型交互节点被激活后，将节点上的仓库组件交给仓库界面显示。
	SetWarehouseComponent(Cast<ULxWarehouseInteractionComponent>(Option.InteractionNode->GetInteractionFeature()));
}

void ULxWarehouseWidget::HandleInteractionCancelled()
{
	SetWarehouseComponent(nullptr);
}

void ULxWarehouseWidget::HandleWarehouseSlotListChanged(const TArray<ULxItemSlotData*>& WarehouseSlots)
{
	WarehouseItemSlotList.Reset();
	for (ULxItemSlotData* SlotData : WarehouseSlots)
	{
		WarehouseItemSlotList.Add(SlotData);
	}

	OnWarehouseItemListUpdated(GetWarehouseItemUIDataList());
}
