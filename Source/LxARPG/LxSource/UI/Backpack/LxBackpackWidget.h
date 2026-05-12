#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxBackpackWidget.generated.h"

class ULxCharacterDataTransferComponent;
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

	/** 请求背包排序。实际操作由数据中转组件转发给背包组件。 */
	UFUNCTION(BlueprintCallable, DisplayName="物品排序")
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, DisplayName="获取物品栏数据列表")
	TArray<UObject*> GetItemUIDataList();

	UFUNCTION(BlueprintCallable, DisplayName="获取装备栏数据列表")
	TArray<UObject*> GetEquipmentUIDataList();

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
};
