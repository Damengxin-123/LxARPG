#include "LxUIManager.h"

#include "Components/CanvasPanelSlot.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/UI/Chat/LxChatWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionUIManager.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxPersistentUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxPopupUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxTogglePanelUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxTooltipUIManager.h"
#include "LxARPG/LxSource/UI/Profession/LxProfessionWidget.h"
#include "LxARPG/LxSource/UI/SkillBackpack/LxSkillBackpackWidget.h"
#include "LxARPG/LxSource/UI/Trade/LxTradeContainerWidget.h"
#include "LxARPG/LxSource/UI/TreasureChest/LxTreasureChestWidget.h"
#include "LxARPG/LxSource/UI/Warehouse/LxWarehouseWidget.h"

void ULxUIManager::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureDefaultManagementObjects();
	InitializeManagementObjects();
}

void ULxUIManager::NativeDestruct()
{
	HUDUIManager = nullptr;
	TogglePanelUIManager = nullptr;
	TooltipUIManager = nullptr;
	InteractionUIManager = nullptr;
	PopupUIManager = nullptr;
	Super::NativeDestruct();
}

void ULxUIManager::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	EnsureDefaultManagementObjects();
	InitializeManagementObjects();
}

void ULxUIManager::SetPlayerController(ALxPlayerController* InPlayerController)
{
	EnsureDefaultManagementObjects();
	m_pPlayerController = InPlayerController;
	InitializeManagementObjects();
	UpdateCursorState();
}

void ULxUIManager::SetControlledCharacter(ALxBaseCharacter* InCharacter)
{
	EnsureDefaultManagementObjects();
	m_pCharacterDataTransferComponent = InCharacter ? InCharacter->GetCharacterDataTransferComponent() : nullptr;
	UpdateUIComponents(m_pCharacterDataTransferComponent);
	if (InteractionUIManager)
	{
		InteractionUIManager->SetPlayerCharacter(Cast<ALxPlayerCharacter>(InCharacter));
	}
	RefreshUI();
}

void ULxUIManager::RefreshUI()
{
	EnsureDefaultManagementObjects();
	InitializeManagementObjects();

	if (HUDUIManager)
	{
		HUDUIManager->RefreshManagedUI();
	}
	if (TogglePanelUIManager)
	{
		TogglePanelUIManager->RefreshManagedUI();
	}
	if (PopupUIManager)
	{
		PopupUIManager->RefreshManagedUI();
	}
	if (InteractionUIManager)
	{
		InteractionUIManager->RefreshManagedUI();
	}

	UpdateCursorState();
}

void ULxUIManager::RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible)
{
	if (!InChildUIWidget)
	{
		return;
	}

	if (ULxItemTooltipWidget* ItemTooltipWidget = Cast<ULxItemTooltipWidget>(InChildUIWidget))
	{
		EnsureDefaultManagementObjects();
		if (TooltipUIManager)
		{
			TooltipUIManager->SetItemTooltipWidget(ItemTooltipWidget, bInShowCursorWhenVisible);
		}
		UpdateCursorState();
		return;
	}

	if (ULxProfessionWidget* ProfessionWidget = Cast<ULxProfessionWidget>(InChildUIWidget))
	{
		RegisterProfessionWidget(ProfessionWidget, bInShowCursorWhenVisible);
		return;
	}

	if (ULxChatWidget* ChatWidget = Cast<ULxChatWidget>(InChildUIWidget))
	{
		RegisterChatWidget(ChatWidget);
		return;
	}

	RegisterTogglePanelWidget(InChildUIWidget, InInputActionID, bInShowCursorWhenVisible);
}

void ULxUIManager::RegisterUIWidget(const FLxUIWidgetRegistration& InRegistration)
{
	if (!InRegistration.UIWidget)
	{
		return;
	}

	EnsureDefaultManagementObjects();
	InitializeManagementObjects();
	InitializeRegisteredUIWidget(InRegistration.UIWidget);

	if (ULxItemTooltipWidget* ItemTooltipWidget = Cast<ULxItemTooltipWidget>(InRegistration.UIWidget))
	{
		EnsureDefaultManagementObjects();
		if (TooltipUIManager)
		{
			TooltipUIManager->SetItemTooltipWidget(ItemTooltipWidget, InRegistration.bShowCursorWhenVisible);
		}
		UpdateCursorState();
		return;
	}

	if (ULxChatWidget* ChatWidget = Cast<ULxChatWidget>(InRegistration.UIWidget))
	{
		RegisterChatWidget(ChatWidget);
		return;
	}

	switch (InRegistration.LayerType)
	{
	case ELxUILayerType::HUD:
		if (HUDUIManager)
		{
			HUDUIManager->RegisterPersistentWidget(InRegistration.UIWidget, InRegistration.bUpdateWithCharacterData);
		}
		break;
	case ELxUILayerType::Panel:
		if (TogglePanelUIManager)
		{
			TogglePanelUIManager->RegisterPanelWidget(
				InRegistration.UIWidget,
				InRegistration.InputActionID,
				InRegistration.bShowCursorWhenVisible,
				InRegistration.bCloseOtherPanelsWhenOpened,
				InRegistration.bUpdateWithCharacterData);
		}
		break;
	case ELxUILayerType::Popup:
		if (PopupUIManager)
		{
			PopupUIManager->RegisterPopupWidget(InRegistration.UIWidget);
		}
		break;
	case ELxUILayerType::Tooltip:
		RegisterItemTooltipWidget(Cast<ULxItemTooltipWidget>(InRegistration.UIWidget));
		break;
	case ELxUILayerType::Interaction:
		if (ULxInteractionEntranceWidget* EntranceWidget = Cast<ULxInteractionEntranceWidget>(InRegistration.UIWidget))
		{
			RegisterInteractionEntranceWidget(EntranceWidget);
		}
		else if (ULxDialogueInteractionWidget* DialogueWidget = Cast<ULxDialogueInteractionWidget>(InRegistration.UIWidget))
		{
			RegisterDialogueInteractionWidget(DialogueWidget);
		}
		else if (ULxWarehouseWidget* WarehouseWidget = Cast<ULxWarehouseWidget>(InRegistration.UIWidget))
		{
			RegisterWarehouseWidget(WarehouseWidget);
		}
		else if (ULxTreasureChestWidget* TreasureChestWidget = Cast<ULxTreasureChestWidget>(InRegistration.UIWidget))
		{
			RegisterTreasureChestWidget(TreasureChestWidget);
		}
		else if (ULxTradeContainerWidget* TradeContainerWidget = Cast<ULxTradeContainerWidget>(InRegistration.UIWidget))
		{
			RegisterTradeContainerWidget(TradeContainerWidget);
		}
		break;
	case ELxUILayerType::Custom:
	default:
		if (HUDUIManager)
		{
			HUDUIManager->RegisterPersistentWidget(InRegistration.UIWidget, InRegistration.bUpdateWithCharacterData);
		}
		break;
	}

	UpdateCursorState();
}

void ULxUIManager::RegisterHUDWidget(ULxUIBaseObject* InChildUIWidget)
{
	FLxUIWidgetRegistration Registration;
	Registration.UIWidget = InChildUIWidget;
	Registration.LayerType = ELxUILayerType::HUD;
	Registration.bShowCursorWhenVisible = false;
	RegisterUIWidget(Registration);
}

void ULxUIManager::RegisterChatWidget(ULxChatWidget* InChatWidget)
{
	if (!InChatWidget)
	{
		return;
	}

	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InChatWidget);
	if (HUDUIManager)
	{
		HUDUIManager->RegisterPersistentWidget(InChatWidget, false);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterTogglePanelWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID,
	bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened)
{
	FLxUIWidgetRegistration Registration;
	Registration.UIWidget = InChildUIWidget;
	Registration.LayerType = ELxUILayerType::Panel;
	Registration.InputActionID = InInputActionID;
	Registration.bShowCursorWhenVisible = bInShowCursorWhenVisible;
	Registration.bCloseOtherPanelsWhenOpened = bInCloseOtherPanelsWhenOpened;
	RegisterUIWidget(Registration);
}

void ULxUIManager::RegisterSkillBackpackWidget(ULxSkillBackpackWidget* InSkillBackpackWidget,
	bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened)
{
	RegisterTogglePanelWidget(
		InSkillBackpackWidget,
		ELxInputActionID::SkillBackpack,
		bInShowCursorWhenVisible,
		bInCloseOtherPanelsWhenOpened);
}

void ULxUIManager::RegisterProfessionWidget(ULxProfessionWidget* InProfessionWidget,
	bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened)
{
	RegisterTogglePanelWidget(
		InProfessionWidget,
		ELxInputActionID::Profession,
		bInShowCursorWhenVisible,
		bInCloseOtherPanelsWhenOpened);
}

void ULxUIManager::RegisterItemTooltipWidget(ULxItemTooltipWidget* InItemTooltipWidget)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InItemTooltipWidget);
	if (TooltipUIManager)
	{
		TooltipUIManager->SetItemTooltipWidget(InItemTooltipWidget);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterInteractionEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InEntranceWidget);
	if (InteractionUIManager)
	{
		InteractionUIManager->RegisterEntranceWidget(InEntranceWidget);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InDialogueInteractionWidget);
	if (InteractionUIManager)
	{
		InteractionUIManager->RegisterDialogueInteractionWidget(InDialogueInteractionWidget);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InWarehouseWidget);
	if (InteractionUIManager)
	{
		InteractionUIManager->RegisterWarehouseWidget(InWarehouseWidget);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InTreasureChestWidget);
	if (InteractionUIManager)
	{
		InteractionUIManager->RegisterTreasureChestWidget(InTreasureChestWidget);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterTradeContainerWidget(ULxTradeContainerWidget* InTradeContainerWidget)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InTradeContainerWidget);
	if (InteractionUIManager)
	{
		InteractionUIManager->RegisterTradeContainerWidget(InTradeContainerWidget);
	}
	UpdateCursorState();
}

void ULxUIManager::RegisterPopupWidget(ULxUIBaseObject* InPopupWidget, bool bInHideOnRegister)
{
	EnsureDefaultManagementObjects();
	InitializeRegisteredUIWidget(InPopupWidget);
	if (PopupUIManager)
	{
		PopupUIManager->RegisterPopupWidget(InPopupWidget, bInHideOnRegister);
	}
	UpdateCursorState();
}

void ULxUIManager::SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible)
{
	EnsureDefaultManagementObjects();

	if (TogglePanelUIManager && TogglePanelUIManager->SetPanelVisible(InChildUIWidget, bInVisible))
	{
		UpdateCursorState();
		return;
	}

	if (PopupUIManager && (bInVisible ? PopupUIManager->ShowPopup(InChildUIWidget) : PopupUIManager->HidePopup(InChildUIWidget)))
	{
		UpdateCursorState();
		return;
	}

	if (!(HUDUIManager && HUDUIManager->ContainsWidget(InChildUIWidget))
		&& !(TooltipUIManager && TooltipUIManager->ContainsWidget(InChildUIWidget))
		&& !(InteractionUIManager && InteractionUIManager->ContainsWidget(InChildUIWidget)))
	{
		return;
	}

	InChildUIWidget->SetVisibility(bInVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	UpdateCursorState();
}

void ULxUIManager::ToggleChildUI(ULxUIBaseObject* InChildUIWidget)
{
	if (!InChildUIWidget)
	{
		return;
	}

	EnsureDefaultManagementObjects();
	if (TogglePanelUIManager && TogglePanelUIManager->TogglePanelWidget(InChildUIWidget))
	{
		UpdateCursorState();
		return;
	}

	SetChildUIVisible(InChildUIWidget, !IsManagedUIVisible(InChildUIWidget));
}

void ULxUIManager::UpdateManagedUIPosition_Implementation(ULxUIBaseObject* InChildUIWidget, FVector2D InScreenPosition)
{
	if (!InChildUIWidget)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(InChildUIWidget->Slot))
	{
		CanvasPanelSlot->SetPosition(InScreenPosition);
	}
}

bool ULxUIManager::ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition)
{
	return ShowItemTooltipWithValue(InItem, 0, false, InMouseScreenPosition);
}

bool ULxUIManager::ShowItemTooltipWithValue(ULxItemBase* InItem, int32 InItemValue, bool bInShowItemValue, FVector2D InMouseScreenPosition)
{
	EnsureDefaultManagementObjects();
	if (TooltipUIManager)
	{
		const bool bResult = TooltipUIManager->ShowItemTooltipWithValue(InItem, InItemValue, bInShowItemValue, InMouseScreenPosition);
		UpdateCursorState();
		return bResult;
	}
	return false;
}

void ULxUIManager::UpdateItemTooltipPosition(FVector2D InMouseScreenPosition)
{
	EnsureDefaultManagementObjects();
	if (TooltipUIManager)
	{
		TooltipUIManager->UpdateItemTooltipPosition(InMouseScreenPosition);
		return;
	}
}

void ULxUIManager::HideItemTooltip()
{
	EnsureDefaultManagementObjects();
	if (TooltipUIManager)
	{
		TooltipUIManager->HideItemTooltip();
		UpdateCursorState();
		return;
	}
}

void ULxUIManager::RefreshCursorState() const
{
	UpdateCursorState();
}

void ULxUIManager::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	EnsureDefaultManagementObjects();
	if (TogglePanelUIManager && TogglePanelUIManager->HandleInputValue(InInputActionID, InValue))
	{
		UpdateCursorState();
		return;
	}
}

void ULxUIManager::EnsureDefaultManagementObjects()
{
	if (!HUDUIManager)
	{
		HUDUIManager = NewObject<ULxPersistentUIManager>(this);
	}
	if (!TogglePanelUIManager)
	{
		TogglePanelUIManager = NewObject<ULxTogglePanelUIManager>(this);
	}
	if (!TooltipUIManager)
	{
		TooltipUIManager = NewObject<ULxTooltipUIManager>(this);
	}
	if (!PopupUIManager)
	{
		PopupUIManager = NewObject<ULxPopupUIManager>(this);
	}
	if (!InteractionUIManager)
	{
		const ULxGameSettings* GameSettings = GetDefault<ULxGameSettings>();
		UClass* InteractionManagerClass = GameSettings ? GameSettings->InteractionUIManagerClass.Get() : nullptr;
		InteractionUIManager = NewObject<ULxInteractionUIManager>(
			this,
			InteractionManagerClass ? InteractionManagerClass : ULxInteractionUIManager::StaticClass());
	}
}

void ULxUIManager::InitializeRegisteredUIWidget(ULxUIBaseObject* InChildUIWidget)
{
	if (InChildUIWidget)
	{
		InChildUIWidget->SetOwningUIManager(this);
	}
}

void ULxUIManager::InitializeManagementObjects()
{
	ULxUIManagementObject* ManagementObjects[] =
	{
		HUDUIManager,
		TogglePanelUIManager,
		TooltipUIManager,
		InteractionUIManager,
		PopupUIManager
	};

	for (ULxUIManagementObject* ManagementObject : ManagementObjects)
	{
		if (!ManagementObject)
		{
			continue;
		}

		ManagementObject->InitializeUIManagement(this);
		ManagementObject->SetPlayerController(m_pPlayerController);
		ManagementObject->SetCharacterDataTransferComponent(m_pCharacterDataTransferComponent);
	}

	if (InteractionUIManager)
	{
		InteractionUIManager->SetPlayerCharacter(Cast<ALxPlayerCharacter>(GetOwningPlayerPawn()));
	}
}

bool ULxUIManager::IsManagedUIVisible(const ULxUIBaseObject* InChildUIWidget) const
{
	return InChildUIWidget && InChildUIWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& InChildUIWidget->GetVisibility() != ESlateVisibility::Hidden;
}

void ULxUIManager::UpdateCursorState() const
{
	if (!m_pPlayerController)
	{
		return;
	}

	if ((TogglePanelUIManager && TogglePanelUIManager->HasVisibleCursorPanel())
		|| (PopupUIManager && PopupUIManager->HasVisiblePopup())
		|| (InteractionUIManager && InteractionUIManager->HasVisibleCursorInteraction())
		|| (TooltipUIManager && TooltipUIManager->ShouldShowCursorForTooltip()))
	{
		m_pPlayerController->ShowCursorFun();
		return;
	}

	m_pPlayerController->HideCursorFun();
}
