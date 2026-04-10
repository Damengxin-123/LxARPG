#include "LxItemGridWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragInfo.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragIconWidget.h"

namespace
{
EItemSlotWidgetType ConvertSlotTypeToWidgetType(const ULxItemSlotData* SlotData)
{
	if (SlotData == nullptr)
	{
		return EItemSlotWidgetType::EIT_None;
	}

	switch (SlotData->ItemSlotType)
	{
	case ELxItemSlotType::Backpack:
	case ELxItemSlotType::Transaction:
	case ELxItemSlotType::TreasureChest:
		return EItemSlotWidgetType::Inventory;
	case ELxItemSlotType::Equipment:
		return EItemSlotWidgetType::Equipment;
	case ELxItemSlotType::Warehouse:
		return EItemSlotWidgetType::Warehouse;
	case ELxItemSlotType::Shortcut:
		return EItemSlotWidgetType::Shortcut;
	default:
		return EItemSlotWidgetType::EIT_None;
	}
}
}

void ULxItemGridWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	InitItemData(ListItemObject);
}

const FLxItemDateBase& ULxItemGridWidget::GetCurrentItemData() const
{
	static const FLxItemDateBase EmptyItemData;

	if (CurrentSlotData == nullptr || !CurrentSlotData->IsValid())
	{
		return EmptyItemData;
	}

	 return *CurrentSlotData->ItemDataPtr->GetItemDataBase();
}

bool ULxItemGridWidget::UseItem() const
{
	if (!CurrentSlotData || !CurrentSlotData->IsValid())
	{
		return false;
	}

	CurrentSlotData->UseItem();
	return true;
}

bool ULxItemGridWidget::ItemIsVaild() const
{
	return CurrentSlotData != nullptr && CurrentSlotData->IsValid();
}

UTexture2D* ULxItemGridWidget::GetDisplayIcon() const
{
	if (ItemIsVaild())
	{
		const FLxItemDateBase& ItemData = GetCurrentItemData();
		if (!ItemData.ItemShowInfo.ItemIcon.IsNull())
		{
			return ItemData.ItemShowInfo.ItemIcon.LoadSynchronous();
		}
	}

	return DefaultIcon.IsNull() ? nullptr : DefaultIcon.LoadSynchronous();
}

FReply ULxItemGridWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return UseItem() ? FReply::Handled() : FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ItemIsVaild() && CurrentSlotData->CanGetItemData())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULxItemGridWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 拖拽发起时，判断当前格子是否为空，或者当前格子类型为不可拖拽的格子
	if (!ItemIsVaild() || !CurrentSlotData->CanGetItemData())
	{
		OutOperation = nullptr;
		return;
	}
	// 创建拖拽事件信息对象
	ULxItemDragInfo* DragOperation = NewObject<ULxItemDragInfo>(this);
	if (DragOperation == nullptr)
	{
		OutOperation = nullptr;
		return;
	}

	// 设置拖拽信息对象参数
	DragOperation->SourceSlot = CurrentSlotData;

	// 创建拖拽时图标
	if (ItemDragIconWidgetClass)
	{
		if (ULxItemDragIconWidget* DragVisual = CreateWidget<ULxItemDragIconWidget>(this, ItemDragIconWidgetClass))
		{
			DragVisual->SetIcon(GetDisplayIcon());
			DragOperation->DefaultDragVisual = DragVisual;
		}
	}

	OutOperation = DragOperation;
}

bool ULxItemGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const ULxItemDragInfo* DragOperation = Cast<ULxItemDragInfo>(InOperation);
	// 判断拖拽事件处理参数是否有效 拖拽数据对象 本格子槽位指针  发起方槽位指针
	if (DragOperation == nullptr || CurrentSlotData == nullptr || DragOperation->SourceSlot == nullptr)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// 判断拖拽双方是否是同一个格子
	if (DragOperation->SourceSlot == CurrentSlotData)
	{
		return false;
	}
	// 调用槽位类型的接口进行堆叠
	switch (CurrentSlotData->ItemEnterToThis(DragOperation->SourceSlot))
	{
		case ELxItemSlotDropResult::Swapped:
		case ELxItemSlotDropResult::StackedAll:
		case ELxItemSlotDropResult::StackedPartial:
		case ELxItemSlotDropResult::EnterSuccess:
			return true;
		default:
			return false;
	}
}

void ULxItemGridWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	// OnItemGridHoverChanged.Broadcast(CurrentItemData, CurrentSlotIndex, CurrentSlotWidgetType, true);
}

void ULxItemGridWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	// OnItemGridHoverChanged.Broadcast(CurrentItemData, CurrentSlotIndex, CurrentSlotWidgetType, false);
}

void ULxItemGridWidget::InitItemData(UObject* ListItemObject)
{
	if (CurrentSlotData)
	{
		CurrentSlotData->OnSlotChanged.RemoveDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}

	CurrentSlotData = nullptr;

	if (ULxItemUIData* BackpackData = Cast<ULxItemUIData>(ListItemObject))
	{
		CurrentSlotData = BackpackData->m_pSlotData;
	}


	if (CurrentSlotData)
	{
		CurrentSlotData->OnSlotChanged.AddDynamic(this, &ULxItemGridWidget::HandleCurrentSlotChanged);
	}
	
}


void ULxItemGridWidget::HandleCurrentSlotChanged() 
{
	BroadcastGridDataChanged();
}

void ULxItemGridWidget::BroadcastGridDataChanged() const
{
	OnItemGridDataChanged.Broadcast();
}
