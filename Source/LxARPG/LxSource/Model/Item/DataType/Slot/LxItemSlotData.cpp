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
	// 调用使用物品函数，如果使用有效，则发出更新事件
	if (ItemDataPtr->UseItem())
	{
		OnSlotChanged.Broadcast();
	}
}

bool ULxItemSlotData::SetItem(ULxItemLogicBase* InItemData)
{
	// 无效的物品数据不能设置进来，空物品视为清空物品槽位
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
		return false;
	}
	return true;
}

ELxItemSlotDropResult ULxItemSlotData::ItemEnterToThis(ULxItemSlotData* InItemSlot)
{
	// 判断对方是否有效，
	if (InItemSlot == nullptr || !InItemSlot->IsValid())
	{
		return ELxItemSlotDropResult::FailedInvalidSource;
	}
	// 判断己方是否能放入物品
	if (ItemSlotType == ELxItemSlotType::None ||
		ItemSlotType == ELxItemSlotType::TreasureChest ||
		ItemSlotType == ELxItemSlotType::Shortcut)
	{
		return ELxItemSlotDropResult::CannotEnter;
	}
	// 判断类型是否符合
	if (!CanAcceptItem(InItemSlot->ItemDataPtr))
	{
		return ELxItemSlotDropResult::TypeError;
	}
	// 经过判断后，确认能放入此槽位
	// 判断此槽位是否为空，为空则直接放入得了
	if (ItemDataPtr == nullptr || !ItemDataPtr->ItemIsValid())
	{
		SetItem(InItemSlot->ItemDataPtr);
		InItemSlot->ClearItem();
		return ELxItemSlotDropResult::EnterSuccess;
	}
	// 双方不为空，则判断是否能堆叠，不能的话就直接
	if (IsStack(InItemSlot->ItemDataPtr))
	{
		// 进行堆叠 堆叠函数会自动计算双方物品数量，并进行数量更改事件发布
		ItemDataPtr->StackItem(InItemSlot->ItemDataPtr);
		return ELxItemSlotDropResult::StackedAll;
	}
	else
	{
		// 交换物品
		ULxItemLogicBase* TempItem = ItemDataPtr;
		SetItem(InItemSlot->ItemDataPtr);
		InItemSlot->SetItem(TempItem);
		return ELxItemSlotDropResult::Swapped;
	}
}

bool ULxEquipmentSlotData::CanAcceptItem(ULxItemLogicBase* InItemData) const
{
	if (InItemData ==  nullptr || !InItemData->ItemIsValid())
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

