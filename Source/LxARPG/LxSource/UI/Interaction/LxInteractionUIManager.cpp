#include "LxInteractionUIManager.h"

#include "LxDialogueInteractionWidget.h"
#include "LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
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

void ULxInteractionUIManager::SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent)
{
	if (PlayerInteractionComponent != InPlayerInteractionComponent)
	{
		if (PlayerInteractionComponent)
		{
			if (FunctionPageWidget) PlayerInteractionComponent->CancelInteraction();
			PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(
				this, &ULxInteractionUIManager::HandleFunctionPageInteractionCancelled);
			PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(
				this, &ULxInteractionUIManager::HandleFunctionPageInteractionActivated);
		}
		HandleFunctionPageInteractionCancelled();
		PlayerInteractionComponent = InPlayerInteractionComponent;
		if (PlayerInteractionComponent)
		{
			PlayerInteractionComponent->OnInteractionCancelled.AddUniqueDynamic(
				this, &ULxInteractionUIManager::HandleFunctionPageInteractionCancelled);
			PlayerInteractionComponent->OnInteractionOptionActivated.AddUniqueDynamic(
				this, &ULxInteractionUIManager::HandleFunctionPageInteractionActivated);
		}
	}
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
		|| IsInteractionWidgetVisible(TradeContainerWidget)
		|| IsInteractionWidgetVisible(FunctionPageWidget);
}

bool ULxInteractionUIManager::OpenFunctionPage(ULxFunctionPageInteractionComponent* InFeature,
	ULxPlayerInteractionModule* InPlayerInteractionComponent)
{
	if (!IsValid(InFeature) || !InFeature->GetPageWidgetClass() || !OwningUIManager
		|| !PlayerInteractionComponent || PlayerInteractionComponent != InPlayerInteractionComponent)
	{
		return false;
	}
	APlayerController* LocalController = PlayerController
		? PlayerController.Get() : OwningUIManager->GetOwningPlayer();
	if (!LocalController || !LocalController->IsLocalController() || !LocalController->GetLocalPlayer())
	{
		return false;
	}

	HandleFunctionPageInteractionCancelled();
	ULxUIBaseObject* NewPage = CreateWidget<ULxUIBaseObject>(LocalController, InFeature->GetPageWidgetClass());
	if (!NewPage) return false;
	FunctionPageFeature = InFeature;
	FunctionPageWidget = NewPage;
	NewPage->SetOwningUIManager(OwningUIManager);
	RefreshWidgetData(NewPage);
	NewPage->SetVisibility(ESlateVisibility::Visible);
	if (!NewPage->AddToPlayerScreen(100))
	{
		HandleFunctionPageInteractionCancelled();
		return false;
	}
	// 页面构造事件允许自行关闭，关闭后不再重新进入功能交互状态。
	if (FunctionPageWidget != NewPage) return false;
	OwningUIManager->RefreshCursorState();
	return true;
}

void ULxInteractionUIManager::CloseFunctionPage()
{
	if (FunctionPageWidget && PlayerInteractionComponent)
	{
		PlayerInteractionComponent->CancelInteraction();
		return;
	}
	HandleFunctionPageInteractionCancelled();
}

bool ULxInteractionUIManager::IsActiveFunctionPageWidget(const ULxUIBaseObject* InWidget) const
{
	return InWidget && FunctionPageWidget == InWidget;
}

void ULxInteractionUIManager::HandleFunctionPageInteractionCancelled()
{
	ULxUIBaseObject* PreviousWidget = FunctionPageWidget;
	ULxFunctionPageInteractionComponent* PreviousFeature = FunctionPageFeature;
	FunctionPageWidget = nullptr;
	FunctionPageFeature = nullptr;
	if (PreviousWidget)
	{
		PreviousWidget->SetVisibility(ESlateVisibility::Collapsed);
		PreviousWidget->RemoveFromParent();
	}
	if (PreviousFeature && PreviousFeature->GetInteractionState() == ELxInteractionDataState::Interacting)
	{
		PreviousFeature->SetInteractionState(ELxInteractionDataState::Interactable);
	}
	if (OwningUIManager) OwningUIManager->RefreshCursorState();
}

void ULxInteractionUIManager::HandleFunctionPageInteractionActivated(const FLxInteractionOption& Option,
	ELxInteractionActionType InteractionType)
{
	if (FunctionPageFeature && (!Option.InteractionNode
		|| Option.InteractionNode->GetInteractionFeature() != FunctionPageFeature))
	{
		// 返回父对话只会广播激活事件，此处释放原页面而不取消已经激活的父节点。
		HandleFunctionPageInteractionCancelled();
	}
}

void ULxInteractionUIManager::RefreshManagedUI()
{
	RefreshInteractionUI();
	RefreshWidgetData(FunctionPageWidget);
}

bool ULxInteractionUIManager::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return EntranceWidget == InWidget
		|| DialogueInteractionWidget == InWidget
		|| WarehouseWidget == InWidget
		|| TreasureChestWidget == InWidget
		|| TradeContainerWidget == InWidget
		|| FunctionPageWidget == InWidget;
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
