#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "LxItemGridWidget.generated.h"

class ALxBaseCharacter;
class ULxCharacterBackpackComponent;
class ULxCharacterEquipmentComponent;
class ULxBackpackData;
class ULxItemData;
class ULxItemDragDropOperation;
class ULxItemDragIconWidget;

/** 当格子物品数据发生变化时通知蓝图刷新显示。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnItemGridDataChanged, ULxItemData*, ItemData, int32, SlotIndex, EItemSlotWidgetType, SlotType, int32, SlotSubType, bool, bHasItem);
/** 当鼠标进入或离开格子时通知蓝图更新悬停状态。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnItemGridHoverChanged, ULxItemData*, ItemData, int32, SlotIndex, EItemSlotWidgetType, SlotType, bool, bIsHovered);
/** 当一次拖放交互处理结束后通知蓝图结果。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnItemGridDropHandled, bool, bSuccess, ULxItemData*, ItemData, EItemSlotWidgetType, SourceType, int32, SourceIndex, EItemSlotWidgetType, TargetType);

/**
 * @brief 物品格子控件
 *
 * C++ 层只负责维护格子数据、响应拖拽和点击等交互逻辑，
 * 不直接操作具体 UI 显示。界面展示通过事件和蓝图实现事件完成。
 */
UCLASS(BlueprintType, Blueprintable)
class LXARPG_API ULxItemGridWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** ListView 设置列表项对象时同步格子数据。 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** 主动设置当前格子绑定的数据对象。 */
	UFUNCTION(BlueprintCallable, Category="Item Grid")
	void SetGridDataObject(ULxBackpackData* InGridDataObject);

	/** 获取当前格子中保存的物品数据。 */
	UFUNCTION(BlueprintPure, Category="Item Grid")
	ULxItemData* GetCurrentItemData() const { return CurrentItemData; }

	/** 获取当前格子的索引。 */
	UFUNCTION(BlueprintPure, Category="Item Grid")
	int32 GetSlotIndex() const { return SlotIndex; }

	/** 获取当前格子的类型。 */
	UFUNCTION(BlueprintPure, Category="Item Grid")
	EItemSlotWidgetType GetSlotWidgetType() const { return SlotWidgetType; }

	/** 获取当前格子的限制子类型。 */
	UFUNCTION(BlueprintPure, Category="Item Grid")
	int32 GetSlotSubType() const { return SlotSubType; }

	/** 使用当前格子中的物品。 */
	UFUNCTION(BlueprintCallable, Category="Item Grid")
	bool UseItem() const;

	/** 拖拽时用于显示图标的蓝图类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Grid")
	TSubclassOf<ULxItemDragIconWidget> ItemDragIconWidgetClass;

	/** 格子数据变化事件，供蓝图更新显示。 */
	UPROPERTY(BlueprintAssignable, Category="Item Grid")
	FOnItemGridDataChanged OnItemGridDataChanged;

	/** 格子悬停状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="Item Grid")
	FOnItemGridHoverChanged OnItemGridHoverChanged;

	/** 格子拖放处理结束事件。 */
	UPROPERTY(BlueprintAssignable, Category="Item Grid")
	FOnItemGridDropHandled OnItemGridDropHandled;

protected:
	/** 处理鼠标按下，用于右键使用和左键开始拖拽。 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	/** 处理拖拽检测，创建拖拽操作对象。 */
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	/** 处理目标格子接收拖放。 */
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	/** 处理鼠标进入格子。 */
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	/** 处理鼠标离开格子。 */
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** 蓝图实现的格子数据刷新事件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Grid")
	void ReceiveGridDataChanged(ULxItemData* ItemData, int32 InSlotIndex, EItemSlotWidgetType InSlotType, int32 InSlotSubType, bool bHasItem);

	/** 蓝图实现的格子悬停状态刷新事件。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Grid")
	void ReceiveGridHoverChanged(ULxItemData* ItemData, int32 InSlotIndex, EItemSlotWidgetType InSlotType, bool bIsHovered);

private:
	/** 绑定当前物品的数据变化事件。 */
	void BindCurrentItem();
	/** 解绑当前物品的数据变化事件。 */
	void UnbindCurrentItem();
	/** 将当前格子数据变化通知给委托和蓝图。 */
	void RefreshGridData();
	/** 判断当前格子是否允许发起拖拽。 */
	bool CanStartDrag() const;
	/** 统一分发拖放处理逻辑。 */
	bool HandleDropOperation(ULxItemDragDropOperation* DragOperation);
	/** 处理拖拽到背包格子的逻辑。 */
	bool HandleInventoryDrop(ULxItemDragDropOperation* DragOperation, ALxBaseCharacter* OwnerCharacter);
	/** 处理拖拽到快捷栏格子的逻辑。 */
	bool HandleShortcutDrop(ULxItemDragDropOperation* DragOperation);
	/** 处理拖拽到装备格子的逻辑。 */
	bool HandleEquipmentDrop(ULxItemDragDropOperation* DragOperation, ALxBaseCharacter* OwnerCharacter);
	/** 处理拖拽到技能格子的逻辑，当前仅保留占位。 */
	bool HandleSkillDrop(ULxItemDragDropOperation* DragOperation);
	/** 处理拖拽到仓库格子的逻辑，当前禁止拖入。 */
	bool HandleWarehouseDrop(ULxItemDragDropOperation* DragOperation);
	/** 判断物品是否满足当前装备格的类型限制。 */
	bool IsEquipmentItemCompatible(ULxItemData* InItemData) const;
	/** 获取当前控件所属的玩家角色。 */
	ALxBaseCharacter* ResolveOwnerCharacter() const;
	/** 获取当前玩家角色的背包组件。 */
	ULxCharacterBackpackComponent* ResolveBackpackComponent(ALxBaseCharacter* OwnerCharacter) const;
	/** 获取当前玩家角色的装备组件。 */
	ULxCharacterEquipmentComponent* ResolveEquipmentComponent(ALxBaseCharacter* OwnerCharacter) const;

	/** 响应当前物品数量或有效性变化。 */
	UFUNCTION()
	void HandleItemQuantityChangeEvent(ULxItemData* InItemData, bool bIsValid);

private:
	/** 当前格子绑定的列表项数据对象。 */
	UPROPERTY()
	TObjectPtr<ULxBackpackData> GridDataObject = nullptr;

	/** 当前格子中关联的物品数据。 */
	UPROPERTY()
	TObjectPtr<ULxItemData> CurrentItemData = nullptr;

	/** 当前格子的索引。 */
	int32 SlotIndex = INDEX_NONE;
	/** 当前格子的功能类型。 */
	EItemSlotWidgetType SlotWidgetType = EItemSlotWidgetType::EIT_None;
	/** 当前格子的限制子类型。 */
	int32 SlotSubType = INDEX_NONE;
};
