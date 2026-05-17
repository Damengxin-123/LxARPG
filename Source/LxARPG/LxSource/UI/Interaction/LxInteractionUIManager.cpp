#include "LxInteractionUIManager.h"

#include "LxDialogueInteractionWidget.h"
#include "LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxPlayerInteractionComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/TreasureChest/LxTreasureChestWidget.h"
#include "LxARPG/LxSource/UI/Warehouse/LxWarehouseWidget.h"

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
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget)
{
	DialogueInteractionWidget = InDialogueInteractionWidget;
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget)
{
	WarehouseWidget = InWarehouseWidget;
	RefreshInteractionUI();
}

void ULxInteractionUIManager::RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget)
{
	TreasureChestWidget = InTreasureChestWidget;
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
}
