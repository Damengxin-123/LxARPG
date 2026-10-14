#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxTradeContainerWidget.generated.h"

class ULxItemSlotData;
class ULxPlayerInteractionModule;
class ULxTradeContainerInteractionComponent;

UCLASS(Blueprintable, BlueprintType, DisplayName="交易容器界面")
class LXARPG_API ULxTradeContainerWidget : public ULxUIBaseObject, public ILxItemQuickTransferInterface
{
	GENERATED_BODY()

public:
	/** 返回当前商城节点地址，商品购买与背包出售共用此交互上下文。 */
	virtual bool GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const override;
	/** 商品格只创建完整实例凭证，库存与金币由服务器买卖模块统一处理。 */
	virtual ULxItemTransferTicket* PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason) override;
	/** 委托交易模块预检当前玩家背包物品的整组出售。 */
	virtual bool CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const override;
	/** 将背包物品快速发送解释为出售请求，保留原有收购业务规则。 */
	virtual bool ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason) override;
	/** 将服务器买卖结果交给蓝图表现。 */
	virtual void NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result) override;
	/** 蓝图可显示交易成功或失败原因，数据仍通过模块事件刷新。 */
	UFUNCTION(BlueprintImplementableEvent, Category="交易UI|物品操作", meta=(DisplayName="交易物品快捷操作完成"))
	void OnItemOperationCompleted(const FLxItemOperationResult& Result);
	/** 保存最近一次买卖结果，方便蓝图查询提示。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="交易UI|物品操作", meta=(DisplayName="最近物品操作结果"))
	FLxItemOperationResult LastItemOperationResult;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent);

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="设置交易容器组件")
	void SetTradeContainerComponent(ULxTradeContainerInteractionComponent* InTradeContainerComponent);

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="关闭交易交互")
	void CloseTradeInteraction();

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="获取交易物品UI数据列表")
	TArray<UObject*> GetTradeItemUIDataList();

	UFUNCTION(BlueprintImplementableEvent, Category="Trade UI", DisplayName="交易物品列表更新")
	void OnTradeItemListUpdated(const TArray<UObject*>& TradeItemUIDataList);

	UFUNCTION(BlueprintImplementableEvent, Category="Trade UI", DisplayName="交易容器打开")
	void OnTradeContainerOpened(ULxTradeContainerInteractionComponent* OpenedTradeContainerComponent);

	UFUNCTION(BlueprintImplementableEvent, Category="Trade UI", DisplayName="交易容器关闭")
	void OnTradeContainerClosed();

private:
	void ShowTradeInteraction();
	void HideTradeInteraction();
	void SetMouseCursorVisible(bool bInVisible);
	void BindPlayerInteractionComponent();
	void UnbindPlayerInteractionComponent();
	void BindTradeContainerComponent();
	void UnbindTradeContainerComponent();
	void RefreshTradeItemList();

	UFUNCTION()
	void HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

	UFUNCTION()
	void HandleInteractionCancelled();

	UFUNCTION()
	void HandleTradeSlotListChanged(const TArray<ULxItemSlotData*>& TradeSlots);

	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionModule> PlayerInteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxTradeContainerInteractionComponent> TradeContainerComponent = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> TradeItemSlotList;
};
