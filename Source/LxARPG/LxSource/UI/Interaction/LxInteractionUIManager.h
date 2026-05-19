#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagementObject.h"
#include "LxInteractionUIManager.generated.h"

class ALxPlayerCharacter;
class ULxDialogueInteractionWidget;
class ULxInteractionEntranceWidget;
class ULxPlayerInteractionComponent;
class ULxTradeContainerWidget;
class ULxTreasureChestWidget;
class ULxWarehouseWidget;

/** 交互 UI 管理器，作为主 UI 管理器的子管理对象，统一持有并初始化交互入口、对话、仓库、宝箱等交互界面。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="交互UI管理器")
class LXARPG_API ULxInteractionUIManager : public ULxUIManagementObject
{
	GENERATED_BODY()

public:
	/** 设置玩家交互组件，并同步给所有已注册的交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionComponent* InPlayerInteractionComponent);

	/** 从玩家角色读取玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家角色")
	void SetPlayerCharacter(ALxPlayerCharacter* InPlayerCharacter);

	/** 注册交互入口 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册交互入口UI")
	void RegisterEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget);

	/** 注册对话交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册对话交互UI")
	void RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget);

	/** 注册仓库交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册仓库UI")
	void RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget);

	/** 注册宝箱交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册宝箱UI")
	void RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget);

	UFUNCTION(BlueprintCallable, Category="浜や簰UI", DisplayName="注册交易容器UI")
	void RegisterTradeContainerWidget(ULxTradeContainerWidget* InTradeContainerWidget);

	/** 刷新所有交互 UI 持有的玩家交互组件引用。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="刷新交互UI")
	void RefreshInteractionUI();

	/** 是否存在需要显示鼠标的可见交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="是否存在需要鼠标的交互UI")
	bool HasVisibleCursorInteraction() const;

	virtual void RefreshManagedUI() override;
	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const override;

private:
	/** 预热交互 UI 的 Slate 控件和布局缓存，避免第一次显示时集中构建导致卡顿。 */
	void PrewarmInteractionWidget(ULxUIBaseObject* InWidget) const;

private:
	/** 当前玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionComponent> PlayerInteractionComponent = nullptr;

	/** 交互入口界面。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractionEntranceWidget> EntranceWidget = nullptr;

	/** 对话交互界面。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxDialogueInteractionWidget> DialogueInteractionWidget = nullptr;

	/** 仓库交互界面。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxWarehouseWidget> WarehouseWidget = nullptr;

	/** 宝箱交互界面。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxTreasureChestWidget> TreasureChestWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxTradeContainerWidget> TradeContainerWidget = nullptr;
};
