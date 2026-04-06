#include "LxItemGridWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemData/LxItemData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxBackpackData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragDropOperation.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragIconWidget.h"

void ULxItemGridWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	SetGridDataObject(Cast<ULxBackpackData>(ListItemObject));
}

void ULxItemGridWidget::SetGridDataObject(ULxBackpackData* InGridDataObject)
{
	UnbindCurrentItem();

	GridDataObject = InGridDataObject;
	CurrentItemData = GridDataObject ? GridDataObject->m_pItemData : nullptr;
	SlotIndex = GridDataObject ? GridDataObject->m_nIndex : INDEX_NONE;
	SlotWidgetType = GridDataObject ? GridDataObject->m_nItemType : EItemSlotWidgetType::EIT_None;
	SlotSubType = GridDataObject ? GridDataObject->m_nItemSubType : INDEX_NONE;

	BindCurrentItem();
	RefreshGridData();
}

bool ULxItemGridWidget::UseItem() const
{
	return CurrentItemData ? CurrentItemData->UseItem() : false;
}

FReply ULxItemGridWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		UseItem();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanStartDrag())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULxItemGridWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (CanStartDrag())
	{
		ULxItemDragDropOperation* DragOperation = NewObject<ULxItemDragDropOperation>(this);
		DragOperation->ItemIndex = SlotIndex;
		DragOperation->SlotWidgetType = SlotWidgetType;
		DragOperation->SlotSubType = SlotSubType;
		DragOperation->ItemData = CurrentItemData;
		DragOperation->SourceWidget = this;

		if (ItemDragIconWidgetClass)
		{
			if (ULxItemDragIconWidget* DragIconWidget = CreateWidget<ULxItemDragIconWidget>(GetWorld(), ItemDragIconWidgetClass))
			{
				DragIconWidget->SetIcon(CurrentItemData->GetItemBase().ItemIconPath.LoadSynchronous());
				DragOperation->DefaultDragVisual = DragIconWidget;
			}
		}

		OutOperation = DragOperation;
	}

	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
}

bool ULxItemGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const bool bHandled = HandleDropOperation(Cast<ULxItemDragDropOperation>(InOperation));
	return bHandled || Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void ULxItemGridWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	OnItemGridHoverChanged.Broadcast(CurrentItemData, SlotIndex, SlotWidgetType, true);
	ReceiveGridHoverChanged(CurrentItemData, SlotIndex, SlotWidgetType, true);
}

void ULxItemGridWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	OnItemGridHoverChanged.Broadcast(CurrentItemData, SlotIndex, SlotWidgetType, false);
	ReceiveGridHoverChanged(CurrentItemData, SlotIndex, SlotWidgetType, false);
}

void ULxItemGridWidget::BindCurrentItem()
{
	if (CurrentItemData)
	{
		CurrentItemData->OnItemQuantityChange.AddDynamic(this, &ULxItemGridWidget::HandleItemQuantityChangeEvent);
	}
}

void ULxItemGridWidget::UnbindCurrentItem()
{
	if (CurrentItemData)
	{
		CurrentItemData->OnItemQuantityChange.RemoveDynamic(this, &ULxItemGridWidget::HandleItemQuantityChangeEvent);
	}
}

void ULxItemGridWidget::RefreshGridData()
{
	const bool bHasItem = CurrentItemData != nullptr && CurrentItemData->IsValid();
	OnItemGridDataChanged.Broadcast(CurrentItemData, SlotIndex, SlotWidgetType, SlotSubType, bHasItem);
	ReceiveGridDataChanged(CurrentItemData, SlotIndex, SlotWidgetType, SlotSubType, bHasItem);
}

bool ULxItemGridWidget::CanStartDrag() const
{
	return CurrentItemData != nullptr
		&& CurrentItemData->IsValid()
		&& SlotWidgetType != EItemSlotWidgetType::Shortcut
		&& SlotWidgetType != EItemSlotWidgetType::Skill;
}

bool ULxItemGridWidget::HandleDropOperation(ULxItemDragDropOperation* DragOperation)
{
	if (!DragOperation || !DragOperation->ItemData)
	{
		return false;
	}

	if (DragOperation->SourceWidget == this)
	{
		return false;
	}

	ALxBaseCharacter* OwnerCharacter = ResolveOwnerCharacter();
	bool bSuccess = false;

	switch (SlotWidgetType)
	{
	case EItemSlotWidgetType::Inventory:
		bSuccess = HandleInventoryDrop(DragOperation, OwnerCharacter);
		break;
	case EItemSlotWidgetType::Shortcut:
		bSuccess = HandleShortcutDrop(DragOperation);
		break;
	case EItemSlotWidgetType::Equipment:
		bSuccess = HandleEquipmentDrop(DragOperation, OwnerCharacter);
		break;
	case EItemSlotWidgetType::Skill:
		bSuccess = HandleSkillDrop(DragOperation);
		break;
	case EItemSlotWidgetType::Warehouse:
		bSuccess = HandleWarehouseDrop(DragOperation);
		break;
	default:
		break;
	}

	OnItemGridDropHandled.Broadcast(bSuccess, DragOperation->ItemData, DragOperation->SlotWidgetType, DragOperation->ItemIndex, SlotWidgetType);
	return bSuccess;
}

bool ULxItemGridWidget::HandleInventoryDrop(ULxItemDragDropOperation* DragOperation, ALxBaseCharacter* OwnerCharacter)
{
	ULxCharacterBackpackComponent* BackpackComponent = ResolveBackpackComponent(OwnerCharacter);
	if (!BackpackComponent || SlotIndex == INDEX_NONE)
	{
		return false;
	}

	switch (DragOperation->SlotWidgetType)
	{
	case EItemSlotWidgetType::Inventory:
		return BackpackComponent->MoveItem(DragOperation->ItemIndex, SlotIndex);
	case EItemSlotWidgetType::Equipment:
		{
			ULxCharacterEquipmentComponent* EquipmentComponent = ResolveEquipmentComponent(OwnerCharacter);
			if (!EquipmentComponent || DragOperation->ItemIndex == INDEX_NONE)
			{
				return false;
			}

			return EquipmentComponent->UnequipItemToBackpackAt(static_cast<ELxEquipmentType>(DragOperation->ItemIndex), SlotIndex);
		}
	case EItemSlotWidgetType::Warehouse:
	case EItemSlotWidgetType::Skill:
	case EItemSlotWidgetType::EIT_None:
		return BackpackComponent->AddItemAtFromExternal(DragOperation->ItemData, SlotIndex);
	default:
		return false;
	}
}

bool ULxItemGridWidget::HandleShortcutDrop(ULxItemDragDropOperation* DragOperation)
{
	if (!GridDataObject || DragOperation->SlotWidgetType == EItemSlotWidgetType::Warehouse)
	{
		return false;
	}

	UnbindCurrentItem();
	GridDataObject->m_pItemData = DragOperation->ItemData;
	CurrentItemData = DragOperation->ItemData;
	BindCurrentItem();
	RefreshGridData();
	return true;
}

bool ULxItemGridWidget::HandleEquipmentDrop(ULxItemDragDropOperation* DragOperation, ALxBaseCharacter* OwnerCharacter)
{
	if (!IsEquipmentItemCompatible(DragOperation->ItemData))
	{
		return false;
	}

	ULxCharacterEquipmentComponent* EquipmentComponent = ResolveEquipmentComponent(OwnerCharacter);
	if (!EquipmentComponent)
	{
		return false;
	}

	const ELxEquipmentType EquipmentType = static_cast<ELxEquipmentType>(SlotSubType);
	switch (DragOperation->SlotWidgetType)
	{
	case EItemSlotWidgetType::Inventory:
		return EquipmentComponent->EquipItemFromBackpackToSlot(DragOperation->ItemIndex, EquipmentType);
	case EItemSlotWidgetType::Warehouse:
	case EItemSlotWidgetType::EIT_None:
		return EquipmentComponent->EquipItemFromExternal(DragOperation->ItemData, EquipmentType);
	default:
		return false;
	}
}

bool ULxItemGridWidget::HandleSkillDrop(ULxItemDragDropOperation* DragOperation)
{
	return false;
}

bool ULxItemGridWidget::HandleWarehouseDrop(ULxItemDragDropOperation* DragOperation)
{
	return false;
}

bool ULxItemGridWidget::IsEquipmentItemCompatible(ULxItemData* InItemData) const
{
	if (!InItemData || InItemData->GetItemType() != ELxItemType::Equipment)
	{
		return false;
	}

	const FLxEquipmentData& EquipmentData = InItemData->GetEquipmentItemData();
	return static_cast<int32>(EquipmentData.EquipmentType) == SlotSubType;
}

ALxBaseCharacter* ULxItemGridWidget::ResolveOwnerCharacter() const
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		return Cast<ALxBaseCharacter>(PlayerController->GetPawn());
	}

	return nullptr;
}

ULxCharacterBackpackComponent* ULxItemGridWidget::ResolveBackpackComponent(ALxBaseCharacter* OwnerCharacter) const
{
	return OwnerCharacter ? OwnerCharacter->GetCharacterBackpackComponent() : nullptr;
}

ULxCharacterEquipmentComponent* ULxItemGridWidget::ResolveEquipmentComponent(ALxBaseCharacter* OwnerCharacter) const
{
	return OwnerCharacter ? OwnerCharacter->GetCharacterEquipmentComponent() : nullptr;
}

void ULxItemGridWidget::HandleItemQuantityChangeEvent(ULxItemData* InItemData, bool bIsValid)
{
	if (InItemData != CurrentItemData)
	{
		return;
	}

	if (!bIsValid)
	{
		UnbindCurrentItem();
		CurrentItemData = nullptr;
		if (GridDataObject)
		{
			GridDataObject->m_pItemData = nullptr;
		}
	}

	RefreshGridData();
}
