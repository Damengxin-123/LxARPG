#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagementObject.h"
#include "LxInteractionUIManager.generated.h"

class ALxPlayerCharacter;
class ULxDialogueInteractionWidget;
class ULxFunctionPageInteractionComponent;
class ULxInteractionEntranceWidget;
class ULxPlayerInteractionModule;
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
	void SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent);

	/** 从玩家角色读取玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家角色")
	void SetPlayerCharacter(ALxPlayerCharacter* InPlayerCharacter);

	/** 注册交互入口 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册交互入口UI")
	void RegisterEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget);

	/** 注册交互对话框，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册交互对话框")
	void RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget);

	/** 注册仓库交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册仓库UI")
	void RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget);

	/** 注册宝箱交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册宝箱UI")
	void RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget);

	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册交易容器UI")
	void RegisterTradeContainerWidget(ULxTradeContainerWidget* InTradeContainerWidget);

	/** 刷新所有交互 UI 持有的玩家交互组件引用。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="刷新交互UI")
	void RefreshInteractionUI();

	/** 是否存在需要显示鼠标的可见交互 UI。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="是否存在需要鼠标的交互UI")
	bool HasVisibleCursorInteraction() const;

	/** 根据交互树资产中的页面类创建当前玩家独立的功能界面。 */
	bool OpenFunctionPage(ULxFunctionPageInteractionComponent* InFeature,
		ULxPlayerInteractionModule* InPlayerInteractionComponent);

	/** 关闭自定义功能界面并结束当前交互，供页面关闭按钮调用。 */
	UFUNCTION(BlueprintCallable, Category="交互UI|功能界面", DisplayName="关闭功能界面")
	void CloseFunctionPage();

	/** 获取当前页面的功能模块，页面蓝图可从中读取页面ID和所属NPC。 */
	UFUNCTION(BlueprintPure, Category="交互UI|功能界面", DisplayName="获取当前功能界面模块")
	ULxFunctionPageInteractionComponent* GetActiveFunctionPageFeature() const { return FunctionPageFeature; }

	/** 获取当前创建的功能页面实例，供蓝图扩展页面初始化。 */
	UFUNCTION(BlueprintPure, Category="交互UI|功能界面", DisplayName="获取当前功能界面")
	ULxUIBaseObject* GetActiveFunctionPageWidget() const { return FunctionPageWidget; }

	/** 判断通用关闭请求是否针对当前自定义功能页面。 */
	bool IsActiveFunctionPageWidget(const ULxUIBaseObject* InWidget) const;

	virtual void RefreshManagedUI() override;
	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const override;

private:
	/** 预热交互 UI 的 Slate 控件和布局缓存，避免第一次显示时集中构建导致卡顿。 */
	void PrewarmInteractionWidget(ULxUIBaseObject* InWidget) const;

	/** 取消交互或替换页面时释放界面并恢复模块的可交互状态。 */
	UFUNCTION(Category="交互UI|功能界面", DisplayName="处理功能界面交互取消")
	void HandleFunctionPageInteractionCancelled();

	/** 返回上级或激活其他节点时关闭原功能页面，保留刚进入的新交互。 */
	UFUNCTION(Category="交互UI|功能界面", DisplayName="处理功能界面交互切换")
	void HandleFunctionPageInteractionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

private:
	/** 当前玩家的自定义功能页面，离开范围或关闭时销毁。 */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="交互UI|功能界面", DisplayName="当前功能界面", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxUIBaseObject> FunctionPageWidget = nullptr;

	/** 当前页面对应的运行时功能模块，不共享其他NPC的状态。 */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="交互UI|功能界面", DisplayName="当前功能界面模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxFunctionPageInteractionComponent> FunctionPageFeature = nullptr;

	/** 当前玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

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
