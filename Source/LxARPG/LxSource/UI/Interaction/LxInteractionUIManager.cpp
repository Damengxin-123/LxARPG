#include "LxInteractionUIManager.h"

#include "LxDialogueInteractionWidget.h"
#include "LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxPlayerInteractionComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Trade/LxTradeContainerWidget.h"
#include "LxARPG/LxSource/UI/TreasureChest/LxTreasureChestWidget.h"
#include "LxARPG/LxSource/UI/Warehouse/LxWarehouseWidget.h"

namespace
{
	bool IsInteractionWidgetVisible(const ULxUIBaseObject* InWidget)
	{
		return InWidget && InWidget->GetVisibility() != ESlateVisibility::Collapsed
			&& InWidget->GetVisibility() != ESlateVisibility::Hidden;
	}
}

void ULxInteractionUIManager::SetPlayerInteractionComponent(ULxPlayerInteractionComponent* InPlayerInteractionComponent)
{
	PlayerInteractionComponent = InPlayerInteractionComponent;
	RefreshInteractionUI();
}

void ULxInteractionUIManager::SetPlayerCharacter(ALxPlayerCharacter* InPlayerCharacter)
{
	SetPlayerInteractionComponent(InPlayerCharacter ? InPlayerCharacter->GetPlayerInteractionComponent() : nullptr);
}

void ULxInteractionUIManager::RegisterEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget)
{
	EntranceWidget = InEntranceWidget;
	PrewarmInteractionWidget(EntranceWidget);
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget)
{
	DialogueInteractionWidget = InDialogueInteractionWidget;
	PrewarmInteractionWidget(DialogueInteractionWidget);
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget)
{
	WarehouseWidget = InWarehouseWidget;
	PrewarmInteractionWidget(WarehouseWidget);
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget)
{
	TreasureChestWidget = InTreasureChestWidget;
	PrewarmInteractionWidget(TreasureChestWidget);
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterTradeContainerWidget(ULxTradeContainerWidget* InTradeContainerWidget)
{
	TradeContainerWidget = InTradeContainerWidget;
	PrewarmInteractionWidget(TradeContainerWidget);
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RefreshInteractionUI()
{
	if (EntranceWidget)
	{
		EntranceWidget->SetPlayerInteractionComponent(PlayerInteractionComponent);
	}

	if (DialogueInteractionWidget)
	{
		DialogueInteractionWidget->SetPlayerInteractionComponent(PlayerInteractionComponent);
	}

	if (WarehouseWidget)
	{
		WarehouseWidget->SetPlayerInteractionComponent(PlayerInteractionComponent);
	}

	if (TreasureChestWidget)
	{
		TreasureChestWidget->SetPlayerInteractionComponent(PlayerInteractionComponent);
	}

	if (TradeContainerWidget)
	{
		TradeContainerWidget->SetPlayerInteractionComponent(PlayerInteractionComponent);
	}
}

bool ULxInteractionUIManager::HasVisibleCursorInteraction() const
{
	return IsInteractionWidgetVisible(DialogueInteractionWidget)
		|| IsInteractionWidgetVisible(WarehouseWidget)
		|| IsInteractionWidgetVisible(TreasureChestWidget)
		|| IsInteractionWidgetVisible(TradeContainerWidget);
}

void ULxInteractionUIManager::RefreshManagedUI()
{
	RefreshInteractionUI();
}

bool ULxInteractionUIManager::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return EntranceWidget == InWidget
		|| DialogueInteractionWidget == InWidget
		|| WarehouseWidget == InWidget
		|| TreasureChestWidget == InWidget
		|| TradeContainerWidget == InWidget;
}

void ULxInteractionUIManager::PrewarmInteractionWidget(ULxUIBaseObject* InWidget) const
{
	if (!InWidget)
	{
		return;
	}

	const ESlateVisibility PreviousVisibility = InWidget->GetVisibility();
	InWidget->SetVisibility(ESlateVisibility::Hidden);
	InWidget->TakeWidget();
	InWidget->ForceLayoutPrepass();
	InWidget->SetVisibility(PreviousVisibility);
}
