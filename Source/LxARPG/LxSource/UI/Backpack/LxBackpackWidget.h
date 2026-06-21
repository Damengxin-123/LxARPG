#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
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
class LXARPG_API ULxBackpackWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
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
};
