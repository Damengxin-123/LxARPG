#include "LxInteractionUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxTogglePanelUIManager.h"
#include "LxARPG/LxSource/UI/Crafting/LxEquipmentCraftingWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxManufacturingUIManager.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxItemManufacturingComponent.h"

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
			PlayerInteractionComponent->OnInteractionCancelled.RemoveAll(this);
			PlayerInteractionComponent->OnInteractionOptionActivated.RemoveAll(this);
		}
		HandleFunctionPageInteractionCancelled();
		PlayerInteractionComponent = InPlayerInteractionComponent;
		if (PlayerInteractionComponent)
		{
			if (!PlayerInteractionComponent->OnInteractionCancelled.IsBoundToObject(this))
			{
				PlayerInteractionComponent->OnInteractionCancelled.AddUObject(this, &ULxInteractionUIManager::HandleFunctionPageInteractionCancelled);
			}
			if (!PlayerInteractionComponent->OnInteractionOptionActivated.IsBoundToObject(this))
			{
				PlayerInteractionComponent->OnInteractionOptionActivated.AddUObject(this, &ULxInteractionUIManager::HandleFunctionPageInteractionActivated);
			}
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
    if (InFeature->IsEquipmentCraftingPage())
    {
        ALxPlayerCharacter* Player = InPlayerInteractionComponent->GetPlayerCharacter();
        ULxItemManufacturingComponent* Component = Player ? Player->GetItemManufacturingComponent() : nullptr;
        ULxManufacturingUIManager* Layer = OwningUIManager->GetManufacturingUIManager();
        auto* CraftPage = Cast<ULxEquipmentCraftingWidget>(NewPage);
        if (!Component || !Layer || !CraftPage || !Component->BeginEquipmentService(InFeature)
            || !Layer->ShowEquipmentPage(CraftPage, Component))
        {
            if (Component) Component->EndEquipmentService();
            HandleFunctionPageInteractionCancelled();
            return false;
        }
        if (OwningUIManager->GetTogglePanelUIManager()) OwningUIManager->GetTogglePanelUIManager()->ShowPanelForInput(ELxInputActionID::Backpack);
        // 背包显示会激活角色面板，最后恢复服务页置顶，保留原来打造界面覆盖背包的顺序。
        OwningUIManager->ActivateRoleWindow(CraftPage);
        OwningUIManager->RefreshCursorState();
        return FunctionPageWidget == NewPage;
    }
    NewPage->SetOwningUIManager(OwningUIManager);
	RefreshWidgetData(NewPage);
	NewPage->SetVisibility(ESlateVisibility::Visible);
	if (!OwningUIManager->MountRoleWindow(NewPage))
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
    if (OwningUIManager && OwningUIManager->GetManufacturingUIManager())
        OwningUIManager->GetManufacturingUIManager()->ReleasePage();
    if (PreviousWidget)
    {
		PreviousWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (OwningUIManager) OwningUIManager->UnmountRoleWindow(PreviousWidget);
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
