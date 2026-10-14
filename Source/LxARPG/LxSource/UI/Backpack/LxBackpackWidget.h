#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxBackpackWidget.generated.h"

class ULxCharacterDataTransferComponent;
class ULxItemGridWidget;
class ULxItemSlotData;

/**
 * 背包界面。
 *
 * UI 不直接订阅背包组件或装备组件，数据获取、筛选和事件刷新都统一经过角色数据中转组件。
 */
UCLASS(Blueprintable, DisplayName="背包界面")
class LXARPG_API ULxBackpackWidget : public ULxUIBaseObject, public ILxItemQuickTransferInterface
{
	GENERATED_BODY()

public:
	/** 返回当前角色的真实背包上下文。 */
	virtual bool GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const override;
	/** 通过角色数据中转组件创建来源提取凭据。 */
	virtual ULxItemTransferTicket* PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason) override;
	/** 委托背包模块检查容量，不在 UI 遍历或修改容器。 */
	virtual bool CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const override;
	/** 将接收请求提交给通用权威入口。 */
	virtual bool ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason) override;
	/** 把完成与失败结果交给蓝图表现。 */
	virtual void NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result) override;
	/** 蓝图展示失败原因或播放音效，不负责修改数量。 */
	UFUNCTION(BlueprintImplementableEvent, Category="背包UI|物品操作", meta=(DisplayName="背包物品快捷操作完成"))
	void OnItemOperationCompleted(const FLxItemOperationResult& Result);
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent) override;
	virtual void NativeDestruct() override;

	/** 背包物品列表变化时调用，蓝图继承后用传入的数据刷新显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="背包UI", DisplayName="物品栏数据更新")
	void OnItemListUpdated(const TArray<UObject*>& ItemUIDataList);

	/** 装备槽位列表变化时调用，蓝图继承后用传入的数据刷新显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="背包UI", DisplayName="装备栏数据更新")
	void OnEquipmentListUpdated(const TArray<UObject*>& EquipmentUIDataList);

	/** 装备槽位数据已准备好时调用，蓝图可在这里按固定布局逐个绑定装备格子。 */
	UFUNCTION(BlueprintImplementableEvent, Category="背包UI|装备栏", DisplayName="请求绑定装备槽位")
	void OnEquipmentSlotBindingRequested();

	/** 请求背包排序。实际操作由数据中转组件转发给背包组件。 */
	UFUNCTION(BlueprintCallable, DisplayName="物品排序")
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, DisplayName="获取物品栏数据列表")
	TArray<UObject*> GetItemUIDataList();

	UFUNCTION(BlueprintCallable, DisplayName="获取装备栏数据列表")
	TArray<UObject*> GetEquipmentUIDataList();

	/** 将指定装备部位的剩余槽位绑定到物品格子；没有找到时清空并隐藏格子。 */
	UFUNCTION(BlueprintCallable, Category="背包UI|装备栏", DisplayName="绑定装备槽位到物品格子", meta=(Categories="物品.装备"))
	bool BindEquipmentSlotToItemGrid(ULxItemGridWidget* ItemGridWidget, FGameplayTag EquipmentTypeTag);

	/** 按物品类型筛选背包显示，空结果也会广播刷新以清空 UI。 */
	UFUNCTION(BlueprintCallable, DisplayName="按照类型过滤显示物品")
	void SwitchItemType(ELxItemType NewType);

private:
	/** 绑定当前角色的数据中转组件并立即拉取一次数据。 */
	void UpdatedBackpack();

	/** 绑定角色数据中转组件，背包 UI 的数据获取和事件刷新都从这里进入。 */
	void BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent);

	/** 解绑数据中转组件事件，避免 UI 销毁后残留监听。 */
	void UnbindDataTransferComponent();

	UFUNCTION()
	void HandleBackpackItemsChanged(const TArray<ULxItemSlotData*>& BackpackItems);

	UFUNCTION()
	void HandleEquipmentSlotsChanged(const TArray<ULxItemSlotData*>& EquipmentSlots);

	/** 当前角色的数据中转组件。 */
	/** 当前 UI 展示用的背包槽位缓存，来源于数据中转组件。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vItemSlotList;

	/** 当前 UI 展示用的装备槽位缓存，来源于数据中转组件。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vEquipmentSlotList;

	/** 当前装备栏手动布局刷新周期内还没有被蓝图绑定走的槽位。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxItemSlotData>> PendingEquipmentSlotBindingList;

	/** 最近快捷操作结果，可供蓝图绑定提示。 */
	UPROPERTY(BlueprintReadOnly, Category="背包UI|物品操作", meta=(AllowPrivateAccess="true", DisplayName="最近物品操作结果"))
	FLxItemOperationResult LastItemOperationResult;
};
