#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxTreasureChestWidget.generated.h"

class ULxItemSlotData;
class ULxPlayerInteractionModule;
class ULxTreasureChestInteractionComponent;

/** 宝箱界面基类。监听玩家交互组件，在宝箱节点激活后显示宝箱槽位列表。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="宝箱界面")
class LXARPG_API ULxTreasureChestWidget : public ULxUIBaseObject, public ILxItemQuickTransferInterface
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 获取当前宝箱节点的安全取出上下文。 */
	virtual bool GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const override;
	/** 委托宝箱数据模块准备完整物品实例的取出凭证。 */
	virtual ULxItemTransferTicket* PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason) override;
	/** 宝箱仅可取出物品，不接受其他窗口存入。 */
	virtual bool CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const override;
	/** 拒绝存入请求，保证来源物品保持不变。 */
	virtual bool ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request,
		ULxUIBaseObject* SourceWindow, FText& OutReason) override;
	/** 缓存安全取出结果，并转发蓝图反馈事件。 */
	virtual void NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result) override;

	/** 最近一次宝箱快捷操作的结果。 */
	UPROPERTY(BlueprintReadOnly, Category="宝箱UI|物品操作", DisplayName="最近物品操作结果")
	FLxItemOperationResult LastItemOperationResult;
	/** 子类蓝图可根据结果提示失败原因或播放取出表现。 */
	UFUNCTION(BlueprintImplementableEvent, Category="宝箱UI|物品操作", DisplayName="物品快捷操作完成")
	void OnItemOperationCompleted(const FLxItemOperationResult& Result);

	/** 设置玩家交互组件，宝箱界面通过它接收宝箱交互节点激活事件。 */
	UFUNCTION(BlueprintCallable, Category="宝箱UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent);

	/** 设置当前界面持有的宝箱组件，通常由交互节点激活事件自动传入。 */
	UFUNCTION(BlueprintCallable, Category="宝箱UI", DisplayName="设置宝箱组件")
	void SetTreasureChestComponent(ULxTreasureChestInteractionComponent* InTreasureChestComponent);

	/** 玩家关闭宝箱 UI 时调用，视为取消当前宝箱交互。 */
	UFUNCTION(BlueprintCallable, Category="宝箱UI", DisplayName="关闭宝箱交互")
	void CloseTreasureChestInteraction();

	/** 获取宝箱物品 UI 数据列表，可直接传给 ListView / TileView。 */
	UFUNCTION(BlueprintCallable, Category="宝箱UI", DisplayName="获取宝箱物品UI数据列表")
	TArray<UObject*> GetTreasureChestItemUIDataList();

	/** 宝箱物品列表刷新时调用，蓝图中负责重建宝箱格子显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="宝箱UI", DisplayName="宝箱物品列表更新")
	void OnTreasureChestItemListUpdated(const TArray<UObject*>& TreasureChestItemUIDataList);

	/** 宝箱被打开时调用，蓝图中可显示界面、播放打开动画。 */
	UFUNCTION(BlueprintImplementableEvent, Category="宝箱UI", DisplayName="宝箱打开")
	void OnTreasureChestOpened(ULxTreasureChestInteractionComponent* OpenedTreasureChestComponent);

	/** 宝箱关闭或交互取消时调用，蓝图中可隐藏界面、播放关闭动画。 */
	UFUNCTION(BlueprintImplementableEvent, Category="宝箱UI", DisplayName="宝箱关闭")
	void OnTreasureChestClosed();

	/** 宝箱物品获取完成时调用，蓝图中可播放完成表现或隐藏入口。 */
	UFUNCTION(BlueprintImplementableEvent, Category="宝箱UI", DisplayName="宝箱获取完成")
	void OnTreasureChestCompleted();

private:
	/** 显示宝箱界面，并切换到适合 UI 操作的输入模式。 */
	void ShowTreasureChestInteraction();
	/** 隐藏宝箱界面，并恢复游戏输入模式。 */
	void HideTreasureChestInteraction();
	/** 设置鼠标显示和输入模式。 */
	void SetMouseCursorVisible(bool bInVisible);

	/** 绑定玩家交互组件事件。 */
	void BindPlayerInteractionComponent();
	/** 解绑玩家交互组件事件。 */
	void UnbindPlayerInteractionComponent();
	/** 绑定当前宝箱组件槽位变化和完成事件。 */
	void BindTreasureChestComponent();
	/** 解绑当前宝箱组件事件。 */
	void UnbindTreasureChestComponent();
	/** 从当前宝箱组件拉取槽位并刷新 UI 数据。 */
	void RefreshTreasureChestItemList();

	UFUNCTION()
	void HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

	UFUNCTION()
	void HandleInteractionCancelled();

	UFUNCTION()
	void HandleTreasureChestSlotListChanged(const TArray<ULxItemSlotData*>& TreasureChestSlots);

	UFUNCTION()
	void HandleTreasureChestCompleted();

	/** 当前玩家交互组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

	/** 当前宝箱界面正在展示的宝箱组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxTreasureChestInteractionComponent> TreasureChestComponent = nullptr;

	/** 当前 UI 显示用宝箱槽位缓存。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> TreasureChestItemSlotList;
};
