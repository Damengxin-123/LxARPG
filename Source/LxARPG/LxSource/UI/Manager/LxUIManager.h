#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagerTypes.h"
#include "LxUIManager.generated.h"

/** 仅供管理器内部派发使用的非蓝图注册数据。 */
struct FLxUIRegistrationData;

class UCanvasPanel;
class ULxItemOperationUIRouter;
class ULxManufacturingUIManager;
class ULxEquipmentCraftingWidget;
class ALxBaseCharacter;
class ALxPlayerController;
class ULxItemBase;
class ULxChatWidget;
class ULxDialogueInteractionWidget;
class ULxItemTooltipWidget;
class ULxInteractionEntranceWidget;
class ULxInteractionUIManager;
class ULxPersistentUIManager;
class ULxPopupUIManager;
class ULxProfessionWidget;
class ULxQuestDetailWidget;
class ULxQuestSummaryWidget;
class ULxTogglePanelUIManager;
class ULxTooltipUIManager;
class ULxTradeContainerWidget;
class ULxTreasureChestWidget;
class ULxWarehouseWidget;
class ULxSkillBackpackWidget;

/** 主 UI 管理器，作为蓝图主界面的统一入口，并把不同表现形式的子 UI 分发给对应管理对象。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="UI管理器")
class LXARPG_API ULxUIManager : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/** 在 Slate 根控件生成前建立兼容旧蓝图的统一窗口承载层。 */
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;

	/** 把角色业务窗口挂到公共父级，保留原画布槽位的布局设置；模态窗口阻挡其他窗口激活。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|角色窗口", DisplayName="挂载角色窗口")
	bool MountRoleWindow(ULxUIBaseObject* InWindow, bool bModal = false);

	/** 从公共窗口层移除窗口并恢复上一个可见窗口。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|角色窗口", DisplayName="卸载角色窗口")
	void UnmountRoleWindow(ULxUIBaseObject* InWindow);

	/** 激活并置顶角色窗口；存在其他模态窗口时拒绝激活。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|角色窗口", DisplayName="激活角色窗口")
	bool ActivateRoleWindow(ULxUIBaseObject* InWindow);

	/** 获取最上层可见业务窗口，可排除来源窗口；不根据物品接收接口筛选。 */
	UFUNCTION(BlueprintPure, Category="UI管理器|角色窗口", DisplayName="获取最上层角色窗口")
	ULxUIBaseObject* GetTopRoleWindow(const ULxUIBaseObject* ExcludeWindow = nullptr) const;

	/** 获取当前激活的可见窗口，隐藏或关闭后自动回到前一个可见窗口。 */
	UFUNCTION(BlueprintPure, Category="UI管理器|角色窗口", DisplayName="获取激活角色窗口")
	ULxUIBaseObject* GetActiveRoleWindow() const;

	/** 判断是否存在阻挡业务操作的可见模态窗口。 */
	UFUNCTION(BlueprintPure, Category="UI管理器|角色窗口", DisplayName="是否存在阻挡模态窗口")
	bool HasBlockingModalWindow() const;

	/** 判断控件本身是否是已挂载的角色业务窗口。 */
	UFUNCTION(BlueprintPure, Category="UI管理器|角色窗口", DisplayName="是否已挂载角色窗口")
	bool IsMountedRoleWindow(const ULxUIBaseObject* InWindow) const;

	/** 获取负责通用快速存取与拆分窗口的操作路由器。 */
	UFUNCTION(BlueprintPure, Category="UI管理器|物品操作", DisplayName="获取物品操作路由器")
	ULxItemOperationUIRouter* GetItemOperationRouter();

	/** 设置当前玩家控制器，用于控制鼠标显示和输入模式。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="设置玩家控制器")
	void SetPlayerController(ALxPlayerController* InPlayerController);

	/** 设置当前受控角色，并同步角色数据到已注册 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="设置受控角色")
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	/** 刷新所有已注册 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="刷新UI")
	void RefreshUI();

    /** 将装备打造界面注册到制造层，服务由NPC交互打开。 */
    UFUNCTION(BlueprintCallable, Category="UI管理器|注册", DisplayName="注册制造界面")
    bool RegisterManufacturingWidget(ULxEquipmentCraftingWidget* InWidget);

	/** 一次注册全部 HUD 子 UI；支持角色状态、快捷栏、Buff、瞄准、聊天、任务简要与自定义界面。返回成功项数。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|注册", meta=(DisplayName="注册HUD子UI"))
	int32 RegisterHUDWidgets(const TArray<FLxHUDUIRegistration>& InRegistrations);

	/** 一次注册全部角色面板；内置功能自动绑定对应开关输入。返回成功项数。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|注册", meta=(DisplayName="注册角色面板子UI"))
	int32 RegisterCharacterPanelWidgets(const TArray<FLxCharacterPanelUIRegistration>& InRegistrations);

	/** 一次注册交互入口、对话、仓库、宝箱和交易子 UI。返回成功项数。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|注册", meta=(DisplayName="注册交互界面子UI"))
	int32 RegisterInteractionWidgets(const TArray<FLxInteractionUIRegistration>& InRegistrations);

	/** 一次注册普通弹窗和物品提示；物品提示仍保留鼠标跟随语义。返回成功项数。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|注册", meta=(DisplayName="注册弹窗子UI"))
	int32 RegisterPopupWidgets(const TArray<FLxPopupUIRegistration>& InRegistrations);

	/** 兼容旧蓝图调用的注册入口，默认按可开关面板处理。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册子UI界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible = true);

	/** 根据注册配置把 UI 分发到对应的管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册UI界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterUIWidget(const FLxUIWidgetRegistration& InRegistration);

	/** 注册 HUD、快捷栏、状态条等常驻 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册HUD界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterHUDWidget(ULxUIBaseObject* InChildUIWidget);

	/** 注册聊天框 UI，固定放入 HUD 层且不跟随角色数据刷新。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册聊天框界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterChatWidget(ULxChatWidget* InChatWidget);

	/** 注册背包、属性等由输入行为打开/关闭的面板 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册按键面板界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterTogglePanelWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false);

	/** 注册技能背包 UI，并由按键面板管理器使用技能面板输入行为开关显示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册技能背包UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterSkillBackpackWidget(ULxSkillBackpackWidget* InSkillBackpackWidget, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false);

	/** 注册职业 UI，并由按键面板管理器使用职业界面输入行为开关显示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册职业UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterProfessionWidget(ULxProfessionWidget* InProfessionWidget, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false);

	/** 注册任务详细界面，使用任务输入行为手动开关。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册任务详细界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterQuestDetailWidget(ULxQuestDetailWidget* InQuestWidget);

	/** 注册任务简要界面为常驻 HUD，不绑定开关输入。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册任务简要界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterQuestSummaryWidget(ULxQuestSummaryWidget* InQuestWidget);

	/** 注册物品悬浮提示 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册物品提示界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterItemTooltipWidget(ULxItemTooltipWidget* InItemTooltipWidget);

	/** 注册交互入口 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册交互入口UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterInteractionEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget);

	/** 注册对话交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册对话交互UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget);

	/** 注册仓库交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册仓库UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget);

	/** 注册宝箱交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册宝箱UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget);

	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册交易容器UI", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterTradeContainerWidget(ULxTradeContainerWidget* InTradeContainerWidget);

	/** 注册弹窗 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|旧版注册", DisplayName="注册弹窗界面", meta=(DeprecatedFunction, DeprecationMessage="请迁移到对应层级的数组注册函数；旧入口仅用于现有蓝图迁移。"))
	void RegisterPopupWidget(ULxUIBaseObject* InPopupWidget, bool bInHideOnRegister = true);

	/** 设置已注册子 UI 的显示状态。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="设置子UI显示状态")
	void SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible);

	/** 切换已注册子 UI 的显示状态。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="切换子UI显示状态")
	void ToggleChildUI(ULxUIBaseObject* InChildUIWidget);

	/** 更新子 UI 在主界面中的位置，默认支持 CanvasPanelSlot，蓝图可重写。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="UI管理器", DisplayName="更新子UI位置")
	void UpdateManagedUIPosition(ULxUIBaseObject* InChildUIWidget, FVector2D InScreenPosition);

	/** 显示物品悬浮提示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="显示物品提示")
	bool ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition);

	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="显示带价值物品提示")
	bool ShowItemTooltipWithValue(ULxItemBase* InItem, int32 InItemValue, bool bInShowItemValue, FVector2D InMouseScreenPosition);

	/** 更新物品悬浮提示的位置。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="更新物品提示位置")
	void UpdateItemTooltipPosition(FVector2D InMouseScreenPosition);

	/** 隐藏物品悬浮提示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="隐藏物品提示")
	void HideItemTooltip();

	/** 根据所有子 UI 管理器的当前显示状态刷新鼠标显示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="刷新鼠标显示状态")
	void RefreshCursorState() const;

	/** 获取 HUD/常驻 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|子管理器", DisplayName="获取HUD管理器")
	ULxPersistentUIManager* GetHUDUIManager() const { return HUDUIManager; }

	/** 获取按键面板 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|子管理器", DisplayName="获取按键面板管理器")
	ULxTogglePanelUIManager* GetTogglePanelUIManager() const { return TogglePanelUIManager; }

	/** 获取悬浮提示 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|子管理器", DisplayName="获取提示管理器")
	ULxTooltipUIManager* GetTooltipUIManager() const { return TooltipUIManager; }

	/** 获取交互 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|子管理器", DisplayName="获取交互管理器")
	ULxInteractionUIManager* GetInteractionUIManager() const { return InteractionUIManager; }

	/** 获取弹窗 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|子管理器", DisplayName="获取弹窗管理器")
    ULxPopupUIManager* GetPopupUIManager() const { return PopupUIManager; }

    /** 获取负责装备打造界面的制造层管理器。 */
    UFUNCTION(BlueprintPure, Category="UI管理|物品制造", DisplayName="获取物品制造UI管理器")
    ULxManufacturingUIManager* GetManufacturingUIManager() const { return ManufacturingUIManager; }

	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

protected:
	/** 可由蓝图提供的公共窗口画布；旧蓝图缺少该控件时由 C++ 自动包裹原布局并创建。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UI管理器|角色窗口", meta=(BindWidgetOptional, DisplayName="角色窗口承载区"))
	TObjectPtr<UCanvasPanel> WindowHost = nullptr;

private:
	/** 由底到顶保存的业务窗口；隐藏窗口保留注册但不参与查询。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxUIBaseObject>> RoleWindows;

	/** 已注册为模态的窗口集合，显隐状态由查询时检查。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxUIBaseObject>> ModalRoleWindows;

	/** 当前主 UI 的物品操作路由器。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxItemOperationUIRouter> ItemOperationRouter = nullptr;

	/** 建立公共画布并保留旧蓝图根布局。 */
	void EnsureRoleWindowHost();
	/** 把物品提示挂到窗口上方的绘制层，不参与业务窗口查询。 */
	bool MountFloatingWidget(ULxUIBaseObject* InWidget);
	/** 按激活顺序和模态优先级同步画布绘制顺序。 */
	void UpdateRoleWindowOrder();
	/** 响应直接设置可见性，窗口重新显示时激活。 */
	void HandleRoleWindowVisibilityChanged(ESlateVisibility InVisibility, ULxUIBaseObject* InWindow);

    /** 装备打造所属的新制造层。 */
    UPROPERTY(Transient)
    TObjectPtr<ULxManufacturingUIManager> ManufacturingUIManager;
	/** 批量注册的唯一派发入口，逐项校验并在批次结束后刷新鼠标。 */
	int32 RegisterLayerWidgets(ELxUILayerType InLayer, const TArray<FLxUIRegistrationData>& InRegistrations);
	/** 检查功能、层级和控件类型，合法时交给现有子管理器。 */
	bool RegisterLayerWidget(ELxUILayerType InLayer, const FLxUIRegistrationData& InRegistration);

	/** 确保默认子管理器已经创建。 */
	void EnsureDefaultManagementObjects();
	/** 绑定已注册 UI 所属的主 UI 管理器。 */
	void InitializeRegisteredUIWidget(ULxUIBaseObject* InChildUIWidget);
	/** 将主 UI 管理器当前上下文同步到全部子管理器。 */
	void InitializeManagementObjects();
	/** 判断 UI 当前是否处于可见状态。 */
	bool IsManagedUIVisible(const ULxUIBaseObject* InChildUIWidget) const;
	/** 根据所有子管理器的状态刷新鼠标显示。 */
	void UpdateCursorState() const;

private:
	/** 当前玩家控制器。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;

	/** HUD/常驻 UI 管理器。 */
	UPROPERTY(Transient, Instanced)
	TObjectPtr<ULxPersistentUIManager> HUDUIManager = nullptr;

	/** 按键开关面板 UI 管理器。 */
	UPROPERTY(Transient, Instanced)
	TObjectPtr<ULxTogglePanelUIManager> TogglePanelUIManager = nullptr;

	/** 悬浮提示 UI 管理器。 */
	UPROPERTY(Transient, Instanced)
	TObjectPtr<ULxTooltipUIManager> TooltipUIManager = nullptr;

	/** 交互 UI 管理器。 */
	UPROPERTY(Transient, Instanced)
	TObjectPtr<ULxInteractionUIManager> InteractionUIManager = nullptr;

	/** 弹窗 UI 管理器。 */
	UPROPERTY(Transient, Instanced)
	TObjectPtr<ULxPopupUIManager> PopupUIManager = nullptr;
};
