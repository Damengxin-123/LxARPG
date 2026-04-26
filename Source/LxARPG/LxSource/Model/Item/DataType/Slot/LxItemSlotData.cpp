#include "LxItemSlotData.h"

#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"

bool ULxItemSlotData::IsEntry() const
{
	return ItemDataPtr == nullptr;
}

bool ULxItemSlotData::IsValid() const
{
	return ItemDataPtr != nullptr && ItemDataPtr->ItemIsValid();
}

void ULxItemSlotData::UseItem()
{
	if (ItemDataPtr && ItemDataPtr->ItemIsValid() && ItemDataPtr->UseItem())
	{
		OnSlotChanged.Broadcast();
	}
}

bool ULxItemSlotData::SetItem(ULxItemLogicBase* InItemData)
{
	if (InItemData != nullptr && !InItemData->ItemIsValid())
	{
		return false;
	}

	ItemDataPtr = InItemData;
	OnSlotChanged.Broadcast();
	return true;
}

void ULxItemSlotData::ClearItem()
{
	ItemDataPtr = nullptr;
	OnSlotChanged.Broadcast();
}

bool ULxItemSlotData::IsStack(ULxItemLogicBase* InItemSlot)
{
	if (InItemSlot == nullptr || !InItemSlot->ItemIsValid() || ItemDataPtr == nullptr || !ItemDataPtr->ItemIsValid())
	{
		return false;
	}
	return ItemDataPtr->ItemIsStack(InItemSlot);
}

bool ULxItemSlotData::CanAcceptItem(ULxItemLogicBase* InItemData) const
{
	if (InItemData == nullptr || !InItemData->ItemIsValid())
	{
		return false;
	}
	switch (ItemSlotType)
	{
	case ELxItemSlotType::None:
		return false;
	case ELxItemSlotType::Backpack:
		return true;
	case ELxItemSlotType::Equipment:
		if (InItemData->GetItemDataBase()->ItemInfo.ItemType != ELxItemType::Equipment)
		{
			return false;
		}
		return true;
	case ELxItemSlotType::Warehouse:
		return true;
	case ELxItemSlotType::TreasureChest:
		return false;
	case ELxItemSlotType::Transaction:
		return true;
	case ELxItemSlotType::Shortcut:
		return true;
	case ELxItemSlotType::BuffDisplay:
		return false;
	}

	return true;
}

bool ULxItemSlotData::CanGetItemData() const
{
	if (ItemDataPtr == nullptr || !ItemDataPtr->ItemIsValid())
	{
		return false;
	}
	switch (ItemSlotType)
	{
	case ELxItemSlotType::None:
		return false;
	case ELxItemSlotType::Backpack:
	case ELxItemSlotType::Equipment:
	case ELxItemSlotType::Warehouse:
	case ELxItemSlotType::TreasureChest:
	case ELxItemSlotType::Transaction:
		return true;
	case ELxItemSlotType::Shortcut:
	case ELxItemSlotType::BuffDisplay:
		return false;
	}
	return true;
}

ELxItemSlotDropResult ULxItemSlotData::ItemEnterToThis(ULxItemSlotData* InItemSlot)
{
	if (InItemSlot == nullptr || !InItemSlot->IsValid())
	{
		return ELxItemSlotDropResult::FailedInvalidSource;
	}

	if (ItemSlotType == ELxItemSlotType::None ||
		ItemSlotType == ELxItemSlotType::TreasureChest ||
		ItemSlotType == ELxItemSlotType::Shortcut ||
		ItemSlotType == ELxItemSlotType::BuffDisplay)
	{
		return ELxItemSlotDropResult::CannotEnter;
	}

	if (!CanAcceptItem(InItemSlot->ItemDataPtr))
	{
		return ELxItemSlotDropResult::TypeError;
	}

	if (ItemDataPtr == nullptr || !ItemDataPtr->ItemIsValid())
	{
		SetItem(InItemSlot->ItemDataPtr);
		InItemSlot->ClearItem();
		return ELxItemSlotDropResult::EnterSuccess;
	}

	if (IsStack(InItemSlot->ItemDataPtr))
	{
		ItemDataPtr->StackItem(InItemSlot->ItemDataPtr);
		return ELxItemSlotDropResult::StackedAll;
	}

	ULxItemLogicBase* TempItem = ItemDataPtr;
	SetItem(InItemSlot->ItemDataPtr);
	InItemSlot->SetItem(TempItem);
	return ELxItemSlotDropResult::Swapped;
}

bool ULxEquipmentSlotData::CanAcceptItem(ULxItemLogicBase* InItemData) const
{
	if (InItemData == nullptr || !InItemData->ItemIsValid())
	{
		return false;
	}
	if (InItemData->GetItemDataBase()->ItemInfo.ItemType != ELxItemType::Equipment)
	{
		return false;
	}
	ULxEquipmentLogic* EquipmentLogic = Cast<ULxEquipmentLogic>(InItemData);
	if (EquipmentLogic == nullptr)
	{
		return false;
	}
	if (EquipmentType != EquipmentLogic->GetEquipmentData()->EquipmentInfo.EquipmentType)
	{
		return false;
	}
	return true;
}

ULxShortcutItemSlotData::ULxShortcutItemSlotData()
{
	ItemSlotType = ELxItemSlotType::Shortcut;
}

bool ULxShortcutItemSlotData::BindSourceSlot(ULxItemSlotData* InSourceSlot)
{
	if (SourceSlot == InSourceSlot)
	{
		HandleSourceSlotChanged();
		return true;
	}

	SourceSlot = InSourceSlot;

	if (BoundItem)
	{
		BoundItem->OnItemInfoChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
	}

	BoundItem = (SourceSlot && SourceSlot->IsValid()) ? SourceSlot->ItemDataPtr : nullptr;
	ItemDataPtr = BoundItem;

	if (BoundItem)
	{
		BoundItem->OnItemInfoChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
		BoundItem->OnItemInfoChanged.AddDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
	}

	OnSlotChanged.Broadcast();
	return true;
}

void ULxShortcutItemSlotData::UseItem()
{
	if (BoundItem && BoundItem->ItemIsValid() && BoundItem->UseItem())
	{
		HandleBoundItemChanged();
		return;
	}

	Super::UseItem();
}

bool ULxShortcutItemSlotData::CanAcceptItem(ULxItemLogicBase* InItemData) const
{
	return InItemData != nullptr && InItemData->ItemIsValid();
}

ELxItemSlotDropResult ULxShortcutItemSlotData::ItemEnterToThis(ULxItemSlotData* InItemSlot)
{
	if (InItemSlot == nullptr || !InItemSlot->IsValid())
	{
		return ELxItemSlotDropResult::FailedInvalidSource;
	}

	if (!CanAcceptItem(InItemSlot->ItemDataPtr))
	{
		return ELxItemSlotDropResult::TypeError;
	}

	BindSourceSlot(InItemSlot);
	return ELxItemSlotDropResult::EnterSuccess;
}

void ULxShortcutItemSlotData::HandleSourceSlotChanged()
{
	ItemDataPtr = (BoundItem && BoundItem->ItemIsValid()) ? BoundItem : nullptr;
	OnSlotChanged.Broadcast();
}

void ULxShortcutItemSlotData::HandleBoundItemChanged()
{
	ItemDataPtr = (BoundItem && BoundItem->ItemIsValid()) ? BoundItem : nullptr;
	OnSlotChanged.Broadcast();
}
