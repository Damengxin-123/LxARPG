#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxWarehouseWidget.generated.h"

class ULxItemSlotData;
class ULxPlayerInteractionModule;
class ULxWarehouseInteractionComponent;

/** 仓库界面基类。监听玩家交互组件，并在仓库节点被激活后显示当前仓库槽位。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="仓库界面")
class LXARPG_API ULxWarehouseWidget : public ULxUIBaseObject, public ILxItemQuickTransferInterface
{
	GENERATED_BODY()

public:
	/** 返回当前打开的仓库功能节点上下文。 */
	virtual bool GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const override;
	/** 对真实来源格子准备安全提取。 */
	virtual ULxItemTransferTicket* PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason) override;
	/** 由仓库模块检查整堆接收条件。 */
	virtual bool CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const override;
	/** 将通用接收请求转交权威数据操作入口。 */
	virtual bool ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason) override;
	/** 通知仓库蓝图播放结果表现。 */
	virtual void NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result) override;
	/** 失败提示由蓝图实现，数量仍从数据模块事件刷新。 */
	UFUNCTION(BlueprintImplementableEvent, Category="仓库UI|物品操作", meta=(DisplayName="仓库物品快捷操作完成"))
	void OnItemOperationCompleted(const FLxItemOperationResult& Result);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 设置玩家交互组件，仓库界面通过它接收仓库交互节点激活事件。 */
	UFUNCTION(BlueprintCallable, Category="仓库UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent);

	/** 设置当前界面持有的仓库组件，通常由交互节点激活事件自动传入。 */
	UFUNCTION(BlueprintCallable, Category="仓库UI", DisplayName="设置仓库组件")
	void SetWarehouseComponent(ULxWarehouseInteractionComponent* InWarehouseComponent);

	/** 玩家关闭仓库 UI 时调用，视为取消当前仓库交互。 */
	UFUNCTION(BlueprintCallable, Category="仓库UI", DisplayName="关闭仓库交互")
	void CloseWarehouseInteraction();

	/** 获取仓库物品 UI 数据列表，可直接传给 ListView / TileView。 */
	UFUNCTION(BlueprintCallable, Category="仓库UI", DisplayName="获取仓库物品UI数据列表")
	TArray<UObject*> GetWarehouseItemUIDataList();

	/** 仓库物品列表刷新时调用，蓝图中负责重建仓库格子显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="仓库UI", DisplayName="仓库物品列表更新")
	void OnWarehouseItemListUpdated(const TArray<UObject*>& WarehouseItemUIDataList);

	/** 仓库被打开时调用，蓝图中可显示界面、播放打开动画。 */
	UFUNCTION(BlueprintImplementableEvent, Category="仓库UI", DisplayName="仓库打开")
	void OnWarehouseOpened(ULxWarehouseInteractionComponent* OpenedWarehouseComponent);

	/** 仓库关闭或交互取消时调用，蓝图中可隐藏界面、播放关闭动画。 */
	UFUNCTION(BlueprintImplementableEvent, Category="仓库UI", DisplayName="仓库关闭")
	void OnWarehouseClosed();

private:
	/** 显示仓库界面，并切换到适合 UI 操作的输入模式。 */
	void ShowWarehouseInteraction();
	/** 隐藏仓库界面，并恢复游戏输入模式。 */
	void HideWarehouseInteraction();
	/** 设置鼠标显示和输入模式。 */
	void SetMouseCursorVisible(bool bInVisible);

	/** 绑定玩家交互组件事件。 */
	void BindPlayerInteractionComponent();
	/** 解绑玩家交互组件事件。 */
	void UnbindPlayerInteractionComponent();
	/** 绑定当前仓库组件槽位变化事件。 */
	void BindWarehouseComponent();
	/** 解绑当前仓库组件槽位变化事件。 */
	void UnbindWarehouseComponent();
	/** 从当前仓库组件拉取槽位并刷新 UI 数据。 */
	void RefreshWarehouseItemList();

	UFUNCTION()
	void HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

	UFUNCTION()
	void HandleInteractionCancelled();

	UFUNCTION()
	void HandleWarehouseSlotListChanged(const TArray<ULxItemSlotData*>& WarehouseSlots);

	/** 当前玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

	/** 当前仓库界面正在展示的仓库组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxWarehouseInteractionComponent> WarehouseComponent = nullptr;

	/** 当前 UI 显示用仓库槽位缓存。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> WarehouseItemSlotList;

	/** 最近操作结果，可供蓝图绑定提示。 */
	UPROPERTY(BlueprintReadOnly, Category="仓库UI|物品操作", meta=(AllowPrivateAccess="true", DisplayName="最近物品操作结果"))
	FLxItemOperationResult LastItemOperationResult;
};
