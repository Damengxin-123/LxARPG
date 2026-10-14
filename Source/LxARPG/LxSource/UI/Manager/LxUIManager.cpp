#include "LxUIManager.h"
#include "LxManufacturingUIManager.h"
#include "LxARPG/LxSource/UI/Crafting/LxEquipmentCraftingWidget.h"

#include "LxARPG/LxSource/UI/Quest/LxQuestDetailWidget.h"
#include "LxARPG/LxSource/UI/Quest/LxQuestSummaryWidget.h"

#include "LxARPG/LxSource/UI/CharacterHUD/LxCharacterStatusWidget.h"
#include "LxARPG/LxSource/UI/ShortcutBar/LxShortcutBarWidget.h"
#include "LxARPG/LxSource/UI/Buff/LxBuffWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemOperationUIRouter.h"
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

/** 内部派发数据，不暴露给蓝图；各层公开注册项只投影本层需要的字段。 */
struct FLxUIRegistrationData
{
	/** 本次注册的控件，仅在同步派发期间使用。 */
	ULxUIBaseObject* UIWidget = nullptr;
	/** HUD 功能，仅在 HUD 层读取。 */
	ELxHUDUIFunction HUDFunction = ELxHUDUIFunction::Custom;
	/** 角色面板功能，仅在角色面板层读取。 */
	ELxCharacterPanelUIFunction CharacterPanelFunction = ELxCharacterPanelUIFunction::Custom;
	/** 交互功能；旧入口无法识别类型时保留无效值以明确报错。 */
	ELxInteractionUIFunction InteractionFunction = static_cast<ELxInteractionUIFunction>(MAX_uint8);
	/** 弹窗功能，仅在弹窗层读取。 */
	ELxPopupUIFunction PopupFunction = ELxPopupUIFunction::Custom;
	/** 自定义角色面板的开关输入。 */
	ELxInputActionID InputActionID = ELxInputActionID::None;
	/** 面板或提示显示时是否开启鼠标；提示新入口采用固定默认值。 */
	bool bShowCursorWhenVisible = true;
	/** 是否关闭其他角色面板。 */
	bool bCloseOtherPanelsWhenOpened = false;
	/** 是否同步角色数据；HUD 新入口固定同步，聊天除外。 */
	bool bUpdateWithCharacterData = true;
	/** 普通弹窗新入口固定初始隐藏；旧入口保留已有参数语义。 */
	bool bHideOnRegister = true;
};

TSharedRef<SWidget> ULxUIManager::RebuildWidget()
{
	if (!WidgetTree) Initialize();
	if (!IsDesignTime()) EnsureRoleWindowHost();
	return Super::RebuildWidget();
}

void ULxUIManager::NativeConstruct()
{
	EnsureRoleWindowHost();
	Super::NativeConstruct();
	EnsureDefaultManagementObjects();
	InitializeManagementObjects();
}

void ULxUIManager::NativeDestruct()
{
	if (ItemOperationRouter) ItemOperationRouter->Shutdown();
	ItemOperationRouter = nullptr;
	if (InteractionUIManager) InteractionUIManager->SetPlayerInteractionComponent(nullptr);
	if (ManufacturingUIManager) ManufacturingUIManager->ReleasePage();
	ManufacturingUIManager = nullptr;
	HUDUIManager = nullptr;
	TogglePanelUIManager = nullptr;
	TooltipUIManager = nullptr;
	InteractionUIManager = nullptr;
	PopupUIManager = nullptr;
	for (ULxUIBaseObject* Window : RoleWindows)
	{
		if (IsValid(Window)) Window->OnNativeVisibilityChanged.RemoveAll(this);
	}
	RoleWindows.Reset();
	ModalRoleWindows.Reset();
	Super::NativeDestruct();
}

void ULxUIManager::EnsureRoleWindowHost()
{
	if (!WidgetTree || WindowHost) return;
	UWidget* OriginalRoot = WidgetTree->RootWidget;
	UCanvasPanel* Wrapper = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("角色界面根"));
	Wrapper->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Wrapper;
	if (OriginalRoot)
	{
		UCanvasPanelSlot* OriginalSlot = Wrapper->AddChildToCanvas(OriginalRoot);
		OriginalSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		OriginalSlot->SetOffsets(FMargin(0.f));
		OriginalSlot->SetZOrder(0);
	}
	WindowHost = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("角色窗口承载区"));
	WindowHost->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* HostSlot = Wrapper->AddChildToCanvas(WindowHost);
	HostSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	HostSlot->SetOffsets(FMargin(0.f));
	HostSlot->SetZOrder(1);
}

bool ULxUIManager::MountFloatingWidget(ULxUIBaseObject* InWidget)
{
	if (!IsValid(InWidget)) return false;
	EnsureRoleWindowHost();
	if (!WindowHost) return false;
	if (InWidget->GetParent() != WindowHost)
	{
		const UCanvasPanelSlot* PreviousSlot = Cast<UCanvasPanelSlot>(InWidget->Slot);
		const bool bHadCanvasSlot = PreviousSlot != nullptr;
		const FAnchorData PreviousLayout = PreviousSlot ? PreviousSlot->GetLayout() : FAnchorData();
		const bool bPreviousAutoSize = PreviousSlot && PreviousSlot->GetAutoSize();
		InWidget->RemoveFromParent();
		UCanvasPanelSlot* NewSlot = WindowHost->AddChildToCanvas(InWidget);
		if (bHadCanvasSlot)
		{
			NewSlot->SetLayout(PreviousLayout);
			NewSlot->SetAutoSize(bPreviousAutoSize);
		}
		else NewSlot->SetAutoSize(true);
	}
	CastChecked<UCanvasPanelSlot>(InWidget->Slot)->SetZOrder(MAX_int32 / 2);
	return true;
}

bool ULxUIManager::MountRoleWindow(ULxUIBaseObject* InWindow, bool bModal)
{
	if (!IsValid(InWindow) || InWindow == this) return false;
	if (!WidgetTree) Initialize();
	EnsureRoleWindowHost();
	if (!WindowHost) return false;
	if (ULxUIManager* PreviousManager = InWindow->GetOwningUIManager(); PreviousManager && PreviousManager != this)
	{
		PreviousManager->UnmountRoleWindow(InWindow);
	}
	if (InWindow->GetParent() != WindowHost)
	{
		// 原画布槽位的锚点、偏移和自动尺寸决定已有蓝图面板布局，重新挂载时完整保留。
		const UCanvasPanelSlot* PreviousSlot = Cast<UCanvasPanelSlot>(InWindow->Slot);
		const bool bHadCanvasSlot = PreviousSlot != nullptr;
		const FAnchorData PreviousLayout = PreviousSlot ? PreviousSlot->GetLayout() : FAnchorData();
		const bool bPreviousAutoSize = PreviousSlot && PreviousSlot->GetAutoSize();
		InWindow->RemoveFromParent();
		UCanvasPanelSlot* NewSlot = WindowHost->AddChildToCanvas(InWindow);
		if (!NewSlot) return false;
		if (bHadCanvasSlot)
		{
			NewSlot->SetLayout(PreviousLayout);
			NewSlot->SetAutoSize(bPreviousAutoSize);
		}
		else
		{
			// 独立功能页已有自己的全屏布局，公共父级继续给予完整可用屏幕。
			NewSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			NewSlot->SetOffsets(FMargin(0.f));
		}
	}
	InWindow->SetOwningUIManager(this);
	RoleWindows.AddUnique(InWindow);
	if (bModal) ModalRoleWindows.AddUnique(InWindow);
	else ModalRoleWindows.Remove(InWindow);
	InWindow->OnNativeVisibilityChanged.RemoveAll(this);
	InWindow->OnNativeVisibilityChanged.AddUObject(this, &ULxUIManager::HandleRoleWindowVisibilityChanged, InWindow);
	if (IsManagedUIVisible(InWindow)) ActivateRoleWindow(InWindow);
	UpdateRoleWindowOrder();
	return true;
}

void ULxUIManager::UnmountRoleWindow(ULxUIBaseObject* InWindow)
{
	if (!InWindow || RoleWindows.Remove(InWindow) == 0) return;
	ModalRoleWindows.Remove(InWindow);
	InWindow->OnNativeVisibilityChanged.RemoveAll(this);
	if (InWindow->GetParent() == WindowHost) InWindow->RemoveFromParent();
	UpdateRoleWindowOrder();
	UpdateCursorState();
}

bool ULxUIManager::ActivateRoleWindow(ULxUIBaseObject* InWindow)
{
	if (!IsMountedRoleWindow(InWindow) || !IsManagedUIVisible(InWindow)) return false;
	for (int32 Index = RoleWindows.Num() - 1; Index >= 0; --Index)
	{
		ULxUIBaseObject* Window = RoleWindows[Index];
		if (IsMountedRoleWindow(Window) && IsManagedUIVisible(Window) && ModalRoleWindows.Contains(Window))
		{
			if (Window != InWindow) return false;
			break;
		}
	}
	RoleWindows.Remove(InWindow);
	RoleWindows.Add(InWindow);
	UpdateRoleWindowOrder();
	return true;
}

ULxUIBaseObject* ULxUIManager::GetTopRoleWindow(const ULxUIBaseObject* ExcludeWindow) const
{
	// 模态窗口始终覆盖普通窗口；其来源被排除时也不允许穿过模态层取得下方目标。
	for (int32 Index = RoleWindows.Num() - 1; Index >= 0; --Index)
	{
		ULxUIBaseObject* Window = RoleWindows[Index];
		if (IsMountedRoleWindow(Window) && IsManagedUIVisible(Window) && ModalRoleWindows.Contains(Window))
			return Window == ExcludeWindow ? nullptr : Window;
	}
	for (int32 Index = RoleWindows.Num() - 1; Index >= 0; --Index)
	{
		ULxUIBaseObject* Window = RoleWindows[Index];
		if (Window != ExcludeWindow && IsMountedRoleWindow(Window) && IsManagedUIVisible(Window)) return Window;
	}
	return nullptr;
}

ULxUIBaseObject* ULxUIManager::GetActiveRoleWindow() const
{
	return GetTopRoleWindow();
}

bool ULxUIManager::HasBlockingModalWindow() const
{
	for (ULxUIBaseObject* Window : ModalRoleWindows)
	{
		if (IsMountedRoleWindow(Window) && IsManagedUIVisible(Window)) return true;
	}
	return false;
}

bool ULxUIManager::IsMountedRoleWindow(const ULxUIBaseObject* InWindow) const
{
	return IsValid(InWindow) && RoleWindows.Contains(InWindow) && WindowHost && InWindow->GetParent() == WindowHost;
}

void ULxUIManager::UpdateRoleWindowOrder()
{
	int32 NormalOrder = 0;
	int32 ModalOrder = RoleWindows.Num();
	for (ULxUIBaseObject* Window : RoleWindows)
	{
		if (IsMountedRoleWindow(Window))
		{
			if (UCanvasPanelSlot* WindowSlot = Cast<UCanvasPanelSlot>(Window->Slot))
				WindowSlot->SetZOrder(ModalRoleWindows.Contains(Window) ? ModalOrder++ : NormalOrder++);
		}
	}
}

void ULxUIManager::HandleRoleWindowVisibilityChanged(ESlateVisibility InVisibility, ULxUIBaseObject* InWindow)
{
	if (InVisibility != ESlateVisibility::Hidden && InVisibility != ESlateVisibility::Collapsed)
		ActivateRoleWindow(InWindow);
	UpdateCursorState();
}

ULxItemOperationUIRouter* ULxUIManager::GetItemOperationRouter()
{
	if (!ItemOperationRouter)
	{
		ItemOperationRouter = NewObject<ULxItemOperationUIRouter>(this);
		ItemOperationRouter->Initialize(this);
	}
	return ItemOperationRouter;
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

int32 ULxUIManager::RegisterHUDWidgets(const TArray<FLxHUDUIRegistration>& InRegistrations)
{
	TArray<FLxUIRegistrationData> Registrations;
	Registrations.Reserve(InRegistrations.Num());
	for (const FLxHUDUIRegistration& Entry : InRegistrations)
	{
		FLxUIRegistrationData& Registration = Registrations.AddDefaulted_GetRef();
		Registration.UIWidget = Entry.UIWidget;
		Registration.HUDFunction = Entry.FunctionType;
	}
	return RegisterLayerWidgets(ELxUILayerType::HUD, Registrations);
}

int32 ULxUIManager::RegisterCharacterPanelWidgets(const TArray<FLxCharacterPanelUIRegistration>& InRegistrations)
{
	TArray<FLxUIRegistrationData> Registrations;
	Registrations.Reserve(InRegistrations.Num());
	for (const FLxCharacterPanelUIRegistration& Entry : InRegistrations)
	{
		FLxUIRegistrationData& Registration = Registrations.AddDefaulted_GetRef();
		Registration.UIWidget = Entry.UIWidget;
		Registration.CharacterPanelFunction = Entry.FunctionType;
		Registration.InputActionID = Entry.InputActionID;
		Registration.bShowCursorWhenVisible = Entry.bShowCursorWhenVisible;
		Registration.bCloseOtherPanelsWhenOpened = Entry.bCloseOtherPanelsWhenOpened;
		Registration.bUpdateWithCharacterData = Entry.bUpdateWithCharacterData;
	}
	return RegisterLayerWidgets(ELxUILayerType::Panel, Registrations);
}

int32 ULxUIManager::RegisterInteractionWidgets(const TArray<FLxInteractionUIRegistration>& InRegistrations)
{
	TArray<FLxUIRegistrationData> Registrations;
	Registrations.Reserve(InRegistrations.Num());
	for (const FLxInteractionUIRegistration& Entry : InRegistrations)
	{
		FLxUIRegistrationData& Registration = Registrations.AddDefaulted_GetRef();
		Registration.UIWidget = Entry.UIWidget;
		Registration.InteractionFunction = Entry.FunctionType;
	}
	return RegisterLayerWidgets(ELxUILayerType::Interaction, Registrations);
}

int32 ULxUIManager::RegisterPopupWidgets(const TArray<FLxPopupUIRegistration>& InRegistrations)
{
	TArray<FLxUIRegistrationData> Registrations;
	Registrations.Reserve(InRegistrations.Num());
	for (const FLxPopupUIRegistration& Entry : InRegistrations)
	{
		FLxUIRegistrationData& Registration = Registrations.AddDefaulted_GetRef();
		Registration.UIWidget = Entry.UIWidget;
		Registration.PopupFunction = Entry.FunctionType;
	}
	return RegisterLayerWidgets(ELxUILayerType::Popup, Registrations);
}

bool ULxUIManager::RegisterManufacturingWidget(ULxEquipmentCraftingWidget* InWidget)
{
    FLxUIRegistrationData Registration; Registration.UIWidget = InWidget;
    return RegisterLayerWidgets(ELxUILayerType::Manufacturing, {Registration}) == 1;
}

int32 ULxUIManager::RegisterLayerWidgets(ELxUILayerType InLayer, const TArray<FLxUIRegistrationData>& InRegistrations)
{
	EnsureDefaultManagementObjects();
	InitializeManagementObjects();
	int32 RegisteredCount = 0;
	for (const FLxUIRegistrationData& Registration : InRegistrations)
	{
		if (RegisterLayerWidget(InLayer, Registration))
		{
			++RegisteredCount;
		}
	}
	UpdateCursorState();
	return RegisteredCount;
}

bool ULxUIManager::RegisterLayerWidget(ELxUILayerType InLayer, const FLxUIRegistrationData& InRegistration)
{
	ULxUIBaseObject* Widget = InRegistration.UIWidget;
	if (!IsValid(Widget))
	{
		UE_LOG(LogTemp, Warning, TEXT("UI注册失败：控件为空或已失效。"));
		return false;
	}

	UClass* RequiredClass = ULxUIBaseObject::StaticClass();
	ELxInputActionID InputAction = InRegistration.InputActionID;
	bool bKnownFunction = true;
	switch (InLayer)
	{
    case ELxUILayerType::Manufacturing:
        RequiredClass = ULxEquipmentCraftingWidget::StaticClass();
        break;
	case ELxUILayerType::HUD:
		switch (InRegistration.HUDFunction)
		{
		case ELxHUDUIFunction::Custom:
			break;
		case ELxHUDUIFunction::CharacterStatus:
			RequiredClass = ULxCharacterStatusWidget::StaticClass();
			break;
		case ELxHUDUIFunction::ShortcutBar:
			RequiredClass = ULxShortcutBarWidget::StaticClass();
			break;
		case ELxHUDUIFunction::Buff:
			RequiredClass = ULxBuffWidget::StaticClass();
			break;
		case ELxHUDUIFunction::Aim:
			break;
		case ELxHUDUIFunction::Chat:
			RequiredClass = ULxChatWidget::StaticClass();
			break;
		case ELxHUDUIFunction::QuestSummary:
			RequiredClass = ULxQuestSummaryWidget::StaticClass();
			break;
		default:
			bKnownFunction = false;
			break;
		}
		break;
	case ELxUILayerType::Panel:
		switch (InRegistration.CharacterPanelFunction)
		{
		case ELxCharacterPanelUIFunction::Custom:
			break;
		case ELxCharacterPanelUIFunction::Backpack:
			InputAction = ELxInputActionID::Backpack;
			break;
		case ELxCharacterPanelUIFunction::CharacterAttribute:
			InputAction = ELxInputActionID::CharacterAttribute;
			break;
		case ELxCharacterPanelUIFunction::SkillBackpack:
			RequiredClass = ULxSkillBackpackWidget::StaticClass();
			InputAction = ELxInputActionID::SkillBackpack;
			break;
		case ELxCharacterPanelUIFunction::Profession:
			RequiredClass = ULxProfessionWidget::StaticClass();
			InputAction = ELxInputActionID::Profession;
			break;
		case ELxCharacterPanelUIFunction::QuestDetail:
			RequiredClass = ULxQuestDetailWidget::StaticClass();
			InputAction = ELxInputActionID::Quest;
			break;
		default:
			bKnownFunction = false;
			break;
		}
		break;
	case ELxUILayerType::Interaction:
		switch (InRegistration.InteractionFunction)
		{
		case ELxInteractionUIFunction::InteractionEntrance:
			RequiredClass = ULxInteractionEntranceWidget::StaticClass();
			break;
		case ELxInteractionUIFunction::Dialogue:
			RequiredClass = ULxDialogueInteractionWidget::StaticClass();
			break;
		case ELxInteractionUIFunction::Warehouse:
			RequiredClass = ULxWarehouseWidget::StaticClass();
			break;
		case ELxInteractionUIFunction::TreasureChest:
			RequiredClass = ULxTreasureChestWidget::StaticClass();
			break;
		case ELxInteractionUIFunction::TradeContainer:
			RequiredClass = ULxTradeContainerWidget::StaticClass();
			break;
		default:
			bKnownFunction = false;
			break;
		}
		break;
	case ELxUILayerType::Popup:
		switch (InRegistration.PopupFunction)
		{
		case ELxPopupUIFunction::Custom:
			break;
		case ELxPopupUIFunction::ItemTooltip:
			RequiredClass = ULxItemTooltipWidget::StaticClass();
			break;
		default:
			bKnownFunction = false;
			break;
		}
		break;
	default:
		bKnownFunction = false;
		break;
	}
	if (!bKnownFunction || !Widget->IsA(RequiredClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("UI注册失败：%s 的功能类型无效或控件类型不匹配，所需类型为 %s。"),
			*GetNameSafe(Widget), *RequiredClass->GetName());
		return false;
	}

	// 同一实例不能同时归属多个管理器，避免刷新和显隐状态互相覆盖。
	const bool bTooltip = InLayer == ELxUILayerType::Popup && InRegistration.PopupFunction == ELxPopupUIFunction::ItemTooltip;
	if ((HUDUIManager->ContainsWidget(Widget) && InLayer != ELxUILayerType::HUD)
		|| (TogglePanelUIManager->ContainsWidget(Widget) && InLayer != ELxUILayerType::Panel)
		|| (InteractionUIManager->ContainsWidget(Widget) && InLayer != ELxUILayerType::Interaction)
		|| (PopupUIManager->ContainsWidget(Widget) && (InLayer != ELxUILayerType::Popup || bTooltip))
		|| (TooltipUIManager->ContainsWidget(Widget) && !bTooltip)
        || (ManufacturingUIManager->ContainsWidget(Widget) && InLayer != ELxUILayerType::Manufacturing))
	{
		UE_LOG(LogTemp, Warning, TEXT("UI注册失败：%s 已注册到其他管理器。"), *GetNameSafe(Widget));
		return false;
	}

	InitializeRegisteredUIWidget(Widget);
	if (bTooltip && !MountFloatingWidget(Widget)) return false;
	const bool bRoleWindow = InLayer == ELxUILayerType::Panel
		|| (InLayer == ELxUILayerType::Interaction && InRegistration.InteractionFunction != ELxInteractionUIFunction::InteractionEntrance)
		|| (InLayer == ELxUILayerType::Popup && !bTooltip);
	if (bRoleWindow && !MountRoleWindow(Widget)) return false;
	switch (InLayer)
	{
    case ELxUILayerType::Manufacturing:
        return ManufacturingUIManager->RegisterEquipmentPage(CastChecked<ULxEquipmentCraftingWidget>(Widget));
	case ELxUILayerType::HUD:
		if (InRegistration.HUDFunction == ELxHUDUIFunction::QuestSummary)
		{
			Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		HUDUIManager->RegisterPersistentWidget(Widget,
			InRegistration.HUDFunction != ELxHUDUIFunction::Chat && InRegistration.bUpdateWithCharacterData);
		return true;
	case ELxUILayerType::Panel:
		if (InRegistration.CharacterPanelFunction == ELxCharacterPanelUIFunction::QuestDetail)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
		TogglePanelUIManager->RegisterPanelWidget(Widget, InputAction, InRegistration.bShowCursorWhenVisible,
			InRegistration.bCloseOtherPanelsWhenOpened, InRegistration.bUpdateWithCharacterData);
		return true;
	case ELxUILayerType::Popup:
		if (bTooltip)
		{
			TooltipUIManager->SetItemTooltipWidget(CastChecked<ULxItemTooltipWidget>(Widget), InRegistration.bShowCursorWhenVisible);
		}
		else
		{
			PopupUIManager->RegisterPopupWidget(Widget, InRegistration.bHideOnRegister);
		}
		return true;
	case ELxUILayerType::Interaction:
		switch (InRegistration.InteractionFunction)
		{
		case ELxInteractionUIFunction::InteractionEntrance:
			InteractionUIManager->RegisterEntranceWidget(CastChecked<ULxInteractionEntranceWidget>(Widget));
			return true;
		case ELxInteractionUIFunction::Dialogue:
			InteractionUIManager->RegisterDialogueInteractionWidget(CastChecked<ULxDialogueInteractionWidget>(Widget));
			return true;
		case ELxInteractionUIFunction::Warehouse:
			InteractionUIManager->RegisterWarehouseWidget(CastChecked<ULxWarehouseWidget>(Widget));
			return true;
		case ELxInteractionUIFunction::TreasureChest:
			InteractionUIManager->RegisterTreasureChestWidget(CastChecked<ULxTreasureChestWidget>(Widget));
			return true;
		case ELxInteractionUIFunction::TradeContainer:
			InteractionUIManager->RegisterTradeContainerWidget(CastChecked<ULxTradeContainerWidget>(Widget));
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}

void ULxUIManager::RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible)
{
	FLxUIWidgetRegistration Registration;
	Registration.UIWidget = InChildUIWidget;
	Registration.InputActionID = InInputActionID;
	Registration.bShowCursorWhenVisible = bInShowCursorWhenVisible;
	if (Cast<ULxProfessionWidget>(InChildUIWidget))
	{
		Registration.InputActionID = ELxInputActionID::Profession;
	}
	RegisterUIWidget(Registration);
}

void ULxUIManager::RegisterUIWidget(const FLxUIWidgetRegistration& InRegistration)
{
	// 旧蓝图只在此适配类型推断；新入口完全由显式功能枚举决定。
	if (!InRegistration.UIWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InRegistration.UIWidget;
	Registration.InputActionID = InRegistration.InputActionID;
	Registration.bShowCursorWhenVisible = InRegistration.bShowCursorWhenVisible;
	Registration.bCloseOtherPanelsWhenOpened = InRegistration.bCloseOtherPanelsWhenOpened;
	Registration.bUpdateWithCharacterData = InRegistration.bUpdateWithCharacterData;
	ELxUILayerType Layer = InRegistration.LayerType;
	if (Cast<ULxItemTooltipWidget>(Registration.UIWidget))
	{
		Layer = ELxUILayerType::Popup;
		Registration.PopupFunction = ELxPopupUIFunction::ItemTooltip;
	}
	else if (Cast<ULxChatWidget>(Registration.UIWidget))
	{
		Layer = ELxUILayerType::HUD;
		Registration.HUDFunction = ELxHUDUIFunction::Chat;
	}
	else if (Layer == ELxUILayerType::Interaction)
	{
		if (Cast<ULxInteractionEntranceWidget>(Registration.UIWidget)) Registration.InteractionFunction = ELxInteractionUIFunction::InteractionEntrance;
		else if (Cast<ULxDialogueInteractionWidget>(Registration.UIWidget)) Registration.InteractionFunction = ELxInteractionUIFunction::Dialogue;
		else if (Cast<ULxWarehouseWidget>(Registration.UIWidget)) Registration.InteractionFunction = ELxInteractionUIFunction::Warehouse;
		else if (Cast<ULxTreasureChestWidget>(Registration.UIWidget)) Registration.InteractionFunction = ELxInteractionUIFunction::TreasureChest;
		else if (Cast<ULxTradeContainerWidget>(Registration.UIWidget)) Registration.InteractionFunction = ELxInteractionUIFunction::TradeContainer;
	}
	else if (Layer == ELxUILayerType::Custom)
	{
		Layer = ELxUILayerType::HUD;
	}
	RegisterLayerWidgets(Layer, {Registration});
}

void ULxUIManager::RegisterHUDWidget(ULxUIBaseObject* InChildUIWidget)
{
	FLxUIWidgetRegistration Registration;
	Registration.UIWidget = InChildUIWidget;
	Registration.LayerType = ELxUILayerType::HUD;
	Registration.bShowCursorWhenVisible = false;
	RegisterUIWidget(Registration);
}

void ULxUIManager::RegisterTogglePanelWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID,
	bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened)
{
	FLxUIWidgetRegistration Registration;
	Registration.UIWidget = InChildUIWidget;
	Registration.InputActionID = InInputActionID;
	Registration.bShowCursorWhenVisible = bInShowCursorWhenVisible;
	Registration.bCloseOtherPanelsWhenOpened = bInCloseOtherPanelsWhenOpened;
	RegisterUIWidget(Registration);
}

void ULxUIManager::RegisterChatWidget(ULxChatWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.HUDFunction = ELxHUDUIFunction::Chat;
	RegisterLayerWidgets(ELxUILayerType::HUD, {Registration});
}

void ULxUIManager::RegisterQuestSummaryWidget(ULxQuestSummaryWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.HUDFunction = ELxHUDUIFunction::QuestSummary;
	RegisterLayerWidgets(ELxUILayerType::HUD, {Registration});
}

void ULxUIManager::RegisterQuestDetailWidget(ULxQuestDetailWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.CharacterPanelFunction = ELxCharacterPanelUIFunction::QuestDetail;
	Registration.bCloseOtherPanelsWhenOpened = true;
	RegisterLayerWidgets(ELxUILayerType::Panel, {Registration});
}

void ULxUIManager::RegisterSkillBackpackWidget(ULxSkillBackpackWidget* InWidget, bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.CharacterPanelFunction = ELxCharacterPanelUIFunction::SkillBackpack;
	Registration.bShowCursorWhenVisible = bInShowCursorWhenVisible;
	Registration.bCloseOtherPanelsWhenOpened = bInCloseOtherPanelsWhenOpened;
	RegisterLayerWidgets(ELxUILayerType::Panel, {Registration});
}

void ULxUIManager::RegisterProfessionWidget(ULxProfessionWidget* InWidget, bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.CharacterPanelFunction = ELxCharacterPanelUIFunction::Profession;
	Registration.bShowCursorWhenVisible = bInShowCursorWhenVisible;
	Registration.bCloseOtherPanelsWhenOpened = bInCloseOtherPanelsWhenOpened;
	RegisterLayerWidgets(ELxUILayerType::Panel, {Registration});
}

void ULxUIManager::RegisterItemTooltipWidget(ULxItemTooltipWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.PopupFunction = ELxPopupUIFunction::ItemTooltip;
	RegisterLayerWidgets(ELxUILayerType::Popup, {Registration});
}

void ULxUIManager::RegisterInteractionEntranceWidget(ULxInteractionEntranceWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.InteractionFunction = ELxInteractionUIFunction::InteractionEntrance;
	RegisterLayerWidgets(ELxUILayerType::Interaction, {Registration});
}

void ULxUIManager::RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.InteractionFunction = ELxInteractionUIFunction::Dialogue;
	RegisterLayerWidgets(ELxUILayerType::Interaction, {Registration});
}

void ULxUIManager::RegisterWarehouseWidget(ULxWarehouseWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.InteractionFunction = ELxInteractionUIFunction::Warehouse;
	RegisterLayerWidgets(ELxUILayerType::Interaction, {Registration});
}

void ULxUIManager::RegisterTreasureChestWidget(ULxTreasureChestWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.InteractionFunction = ELxInteractionUIFunction::TreasureChest;
	RegisterLayerWidgets(ELxUILayerType::Interaction, {Registration});
}

void ULxUIManager::RegisterTradeContainerWidget(ULxTradeContainerWidget* InWidget)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.InteractionFunction = ELxInteractionUIFunction::TradeContainer;
	RegisterLayerWidgets(ELxUILayerType::Interaction, {Registration});
}

void ULxUIManager::RegisterPopupWidget(ULxUIBaseObject* InWidget, bool bInHideOnRegister)
{
	if (!InWidget) return;
	FLxUIRegistrationData Registration;
	Registration.UIWidget = InWidget;
	Registration.PopupFunction = ELxPopupUIFunction::Custom;
	Registration.bHideOnRegister = bInHideOnRegister;
	RegisterLayerWidgets(ELxUILayerType::Popup, {Registration});
}

void ULxUIManager::SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible)
{
	EnsureDefaultManagementObjects();

	// 功能页面使用通用关闭按钮时，也需要结束交互并释放NPC的功能状态。
	if (!bInVisible && InteractionUIManager && InteractionUIManager->IsActiveFunctionPageWidget(InChildUIWidget))
	{
		InteractionUIManager->CloseFunctionPage();
		UpdateCursorState();
		return;
	}

    if (!bInVisible && ManufacturingUIManager && ManufacturingUIManager->ContainsWidget(InChildUIWidget))
    {
        ManufacturingUIManager->ReleasePage();
        UpdateCursorState();
        return;
    }
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
    if (!ManufacturingUIManager) ManufacturingUIManager = NewObject<ULxManufacturingUIManager>(this);
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
		PopupUIManager,
        ManufacturingUIManager
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

	if (HasBlockingModalWindow() || (ManufacturingUIManager && ManufacturingUIManager->HasVisiblePage())
        || (TogglePanelUIManager && TogglePanelUIManager->HasVisibleCursorPanel())
		|| (PopupUIManager && PopupUIManager->HasVisiblePopup())
		|| (InteractionUIManager && InteractionUIManager->HasVisibleCursorInteraction())
		|| (TooltipUIManager && TooltipUIManager->ShouldShowCursorForTooltip()))
	{
		m_pPlayerController->ShowCursorFun();
		return;
	}

	m_pPlayerController->HideCursorFun();
}
