#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "LxItemGridWidget.generated.h"

class ULxItemUIData;
class ULxItemBase;
class ULxItemSlotData;
class ULxItemDragInfo;
class ULxItemDragIconWidget;
class ULxUIManager;
class UTexture2D;

/**
 * @brief 物品格子控件
 *
 * C++ 层只负责维护格子数据、响应拖拽和点击等交互逻辑，
 * 不直接操作具体 UI 显示。界面展示通过事件和蓝图实现事件完成。
 */
UCLASS(BlueprintType, Blueprintable, DisplayName="物品格子控件")
class LXARPG_API ULxItemGridWidget : public ULxUIBaseObject, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/**
	 * @brief 当列表项对象设置时调用，用于初始化或更新当前格子的数据。
	 * 该方法在列表项对象被设置到当前格子时被调用，可以用来根据新的列表项对象更新格子的显示或其他相关数据。
	 * @param ListItemObject 指向新设置的列表项对象的指针，通常是一个包含物品信息的对象。
	 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

	/**
	 * @brief 获取当前格子的槽位类型。
	 *
	 * @return 返回一个ELxItemSlotType类型的引用，表示当前格子的槽位类型。如果没有设置，则返回ELxItemSlotType::None。
	 */
	UFUNCTION(BlueprintPure, DisplayName="获取槽位类型")
	ELxItemSlotType GetSlotType() const;
	//
	// /**
	//  * @brief 获取当前格子中物品的类型。
	//  *
	//  * 该方法检查当前格子中的物品数据，并根据物品槽位类型返回相应的物品类型。
	//  * 如果当前格子没有物品数据或物品类型无法识别，则返回0。
	//  *
	//  * @return 返回一个整数，表示当前格子中物品的类型。如果无物品或类型未知，则返回0。
	//  */
	// UFUNCTION(BlueprintPure, DisplayName="获取槽位内物品类型")
	// int32 GetItemType() const;
	
	
	/**
	 * @brief 使用当前格子中的物品。
	 * 该方法尝试使用当前格子中存放的物品。如果成功使用，则返回`true`；否则返回`false`。
	 * @return 如果成功使用了物品则返回`true`，否则返回`false`。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Grid")
	bool UseItem() const;

	UFUNCTION(BlueprintCallable, Category="Item Grid", DisplayName="设置槽位数据")
	void SetItemSlotData(ULxItemSlotData* InSlotData);
	
	/**
	 * @brief 检查当前格子中的物品是否有效。
	 * 该方法用于判断当前格子中是否有有效的物品数据。如果`CurrentSlotData`不为空且其`IsValid`方法返回`true`，则认为当前格子中的物品是有效的。
	 * @return 如果当前格子中的物品有效，则返回`true`；否则返回`false`。
	 */
	UFUNCTION(BlueprintPure, Category="Item Grid")
	bool ItemIsVaild() const;
	
	/** 获取当前格子应显示的图标，没有物品时返回默认图标。 */
	UFUNCTION(BlueprintPure, Category="Item Grid", DisplayName="获取显示图标")
	UTexture2D* GetDisplayIcon() const;
	
	/** 获取当前物品的可视化名称，空格子返回空文本。 */
	UFUNCTION(BlueprintPure, Category="Item Grid", DisplayName="获取物品可视化名称")
	FText GetItemDisplayName() const;
	
	/** 获取当前物品的可视化描述，空格子返回空文本。 */
	UFUNCTION(BlueprintPure, Category="Item Grid", DisplayName="获取物品可视化描述")
	FText GetItemDisplayDescription() const;
	
	/**
	 * 设置网格的默认图标。
	 *
	 * @param InDefaultIcon 用于设置为默认图标的纹理对象。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Grid", DisplayName="设置默认图标")
	void SetDefaultIcon(UTexture2D* InDefaultIcon);
	
	/**
	 * @brief 获取当前格子中装备的类型。
	 *
	 * 该方法检查当前格子中的物品数据，并根据物品槽位类型返回相应的装备类型。
	 * 如果当前格子没有物品数据或物品类型无法识别，则返回false。
	 *
	 * @param OutEquipmentType 输出参数，用于接收当前格子中装备的类型。
	 * @return 返回一个布尔值，表示是否成功获取到装备类型。如果成功获取到装备类型则返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Grid", DisplayName="获取装备部位")
	bool GetEquipmentType(FGameplayTag& OutEquipmentType) const;
	
	/**
	 * @brief 获取当前格子中物品的数量。
	 *
	 * 该方法首先检查物品是否有效。如果有效，则获取当前物品的数据，并检查物品图标是否为空。
	 * 如果物品图标不为空，则返回物品数量；否则，返回0。
	 *
	 * @return 当前格子中物品的数量。如果物品无效或图标为空，则返回0。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Grid", DisplayName="获取物品数量")
	FText GetItemCount() const;
	
	/**
	 * @brief 获取当前格子中物品的稀有度颜色
	 *
	 * 该方法用于获取当前格子中显示物品的稀有度颜色。如果格子中的物品有效且具有稀有度信息，则返回对应的稀有度颜色；否则，返回白色。
	 *
	 * @return 返回一个FLinearColor对象，表示物品的稀有度颜色。若无有效物品或稀有度信息，则返回白色。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Grid", DisplayName="获取物品稀有度颜色")
	ELxItemRarityType GetItemRarity() const;


	/** 物品显示需要整体刷新时调用，蓝图中应同时更新图标和数量显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Grid", DisplayName="物品显示更新")
	void OnItemDisplayUpdated(UTexture2D* DisplayIcon, const FText& ItemName, const FText& ItemDescription, const FText& ItemCount, ELxItemRarityType Rarity);

	/** 空装备槽位刷新时调用，蓝图中可根据装备部位设置默认图标。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Grid", DisplayName="空装备槽位显示更新")
	void OnEmptyEquipmentSlotUpdated(bool IsEquipmentSlot, FGameplayTag EquipmentType);

	/** 只有物品数量变化时调用，蓝图中只更新数量显示即可。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Grid", DisplayName="物品数量更新")
	void OnItemCountUpdated(const FText& ItemCount);

	UFUNCTION(BlueprintImplementableEvent, Category="Item Grid", DisplayName="空格子显示更新")
	void OnItemIsEmpty();
	

protected:
	/**
	 * @brief 拖拽UI类型
	 * 该属性定义了在拖拽物品时使用的UI小部件类。通过设置此属性，可以指定一个自定义的`ULxItemDragIconWidget`子类来作为拖拽操作中的视觉表示。
	 * 该属性可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="拖拽UI类型")
	TSubclassOf<ULxItemDragIconWidget> ItemDragIconWidgetClass;

	/** 空槽位时显示的默认图标，由蓝图决定如何使用。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item Grid", DisplayName="默认图标")
	TSoftObjectPtr<UTexture2D> DefaultIcon;

	/**
	 * @brief 处理鼠标按钮按下事件。
	 * 该方法在鼠标按钮在当前格子上被按下时调用，可以用来实现鼠标点击响应逻辑，例如选择格子或开始拖拽操作等。
	 * @param InGeometry 当前小部件的几何信息。
	 * @param InMouseEvent 鼠标事件信息，包括按下的按钮、位置等。
	 * @return 返回一个`FReply`对象，用于处理输入事件的结果。可以通过返回不同的`FReply`来控制进一步的用户交互行为。
	 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	/**
	 * @brief 当检测到拖拽操作时调用此方法。
	 * 该方法在用户开始拖拽当前格子中的物品时被触发，用于初始化并返回一个`UDragDropOperation`对象来处理拖拽过程。
	 * @param InGeometry 当前小部件的几何信息。
	 * @param InMouseEvent 触发拖拽操作的鼠标事件信息。
	 * @param OutOperation 输出参数，用于接收创建的`UDragDropOperation`实例，该实例将管理整个拖拽操作。
	 */
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	/**
	 * @brief 处理拖拽释放事件。
	 * 该方法在用户将拖拽的物品释放到当前格子时被调用，用于处理放置逻辑并决定是否接受放置的物品。
	 * @param InGeometry 当前小部件的几何信息。
	 * @param InDragDropEvent 拖拽事件信息，包括拖拽源、目标等。
	 * @param InOperation 拖拽操作对象，包含了拖拽过程中所需的所有信息和状态。
	 * @return 返回一个布尔值表示是否成功处理了拖拽释放。如果返回`true`，则表示接受了放置的物品；如果返回`false`，则表示拒绝了放置。
	 */
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	/**
	 * @brief 当鼠标进入当前格子时调用此方法。
	 * 该方法在鼠标指针进入当前格子区域时被触发，可以用来实现鼠标悬停效果，如显示提示信息或改变格子样式等。
	 * @param InGeometry 当前小部件的几何信息。
	 * @param InMouseEvent 鼠标事件信息，包括位置、按钮状态等。
	 */
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	/**
	 * @brief 当鼠标离开当前格子时调用此方法。
	 * 该方法在鼠标指针离开当前格子区域时被触发，可以用来实现鼠标悬停效果的取消，如隐藏提示信息或恢复格子样式等。
	 * @param InMouseEvent 鼠标事件信息，包括位置、按钮状态等。
	 */
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;


private:
	ULxUIManager* GetUIManager() const;

	void ShowItemTooltip(const FVector2D& InMouseScreenPosition) const;

	void UpdateItemTooltipPosition(const FVector2D& InMouseScreenPosition) const;

	void HideItemTooltip() const;

	void InitItemData(UObject* ListItemObject);

	/** * 处理当前槽位发生变化的事件。
	 * 该方法会在当前槽位数据发生改变时被调用，用于刷新绑定的物品信息并广播格子数据变化。
	 */
	UFUNCTION()
	void HandleCurrentSlotChanged(ULxItemBase* InItemData);

	UFUNCTION()
	void HandleCurrentItemChanged(ULxItemBase* Item);

	void BroadcastGridDataChanged();

	void BroadcastItemCountChanged();

	
	
	/** 指向当前格子绑定的槽位数据对象。用于存储和管理与该格子关联的具体数据。 */
	UPROPERTY()
	TObjectPtr<ULxItemSlotData> CurrentSlotData = nullptr;
	
	void SetCurrentSlotDataInternal(ULxItemSlotData* InSlotData);
};
