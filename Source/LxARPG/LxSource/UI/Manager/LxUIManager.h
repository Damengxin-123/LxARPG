#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagerTypes.h"
#include "LxUIManager.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxItemBase;
class ULxDialogueInteractionWidget;
class ULxItemTooltipWidget;
class ULxInteractionEntranceWidget;
class ULxInteractionUIManager;
class ULxPersistentUIManager;
class ULxPopupUIManager;
class ULxProfessionWidget;
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
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;

	/** 设置当前玩家控制器，用于控制鼠标显示和输入模式。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="设置玩家控制器")
	void SetPlayerController(ALxPlayerController* InPlayerController);

	/** 设置当前受控角色，并同步角色数据到已注册 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="设置受控角色")
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	/** 刷新所有已注册 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="刷新UI")
	void RefreshUI();

	/** 兼容旧蓝图调用的注册入口，默认按可开关面板处理。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册子UI界面")
	void RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible = true);

	/** 根据注册配置把 UI 分发到对应的管理器。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册UI界面")
	void RegisterUIWidget(const FLxUIWidgetRegistration& InRegistration);

	/** 注册 HUD、快捷栏、状态条等常驻 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册HUD界面")
	void RegisterHUDWidget(ULxUIBaseObject* InChildUIWidget);

	/** 注册背包、属性等由输入行为打开/关闭的面板 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册按键面板界面")
	void RegisterTogglePanelWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false);

	/** 注册技能背包 UI，并由按键面板管理器使用技能面板输入行为开关显示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册技能背包UI")
	void RegisterSkillBackpackWidget(ULxSkillBackpackWidget* InSkillBackpackWidget, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false);

	/** 注册职业 UI，并由按键面板管理器使用职业界面输入行为开关显示。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册职业UI")
	void RegisterProfessionWidget(ULxProfessionWidget* InProfessionWidget, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false);

	/** 注册物品悬浮提示 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册物品提示界面")
	void RegisterItemTooltipWidget(ULxItemTooltipWidget* InItemTooltipWidget);

	/** 注册交互入口 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|交互UI", DisplayName="注册交互入口UI")
	void RegisterInteractionEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget);

	/** 注册对话交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|交互UI", DisplayName="注册对话交互UI")
	void RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget);

	/** 注册仓库交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|交互UI", DisplayName="注册仓库UI")
	void RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget);

	/** 注册宝箱交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器|交互UI", DisplayName="注册宝箱UI")
	void RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget);

	UFUNCTION(BlueprintCallable, Category="UI管理器|交互UI", DisplayName="注册交易容器UI")
	void RegisterTradeContainerWidget(ULxTradeContainerWidget* InTradeContainerWidget);

	/** 注册弹窗 UI。 */
	UFUNCTION(BlueprintCallable, Category="UI管理器", DisplayName="注册弹窗界面")
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

	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

private:
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
