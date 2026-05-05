#include "LxItemSlotData.h"

#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"



bool ULxItemSlotData::IsValid() const
{
	return ItemDataPtr != nullptr && ItemDataPtr->ItemIsValid();
}

void ULxItemSlotData::UseItem()
{
	if (!IsValid())
	{
		return;
	}

	const ELxItemUseState UseState = ItemDataPtr->ItemUse();
	if (UseState == ELxItemUseState::Failed)
	{
		return;
	}

	if (UseState == ELxItemUseState::ActivateEntry)
	{
		if (ULxCharacterBackpackComponent* BackpackComponent = GetTypedOuter<ULxCharacterBackpackComponent>())
		{
			BackpackComponent->NotifyItemUsedFromSlot(ItemDataPtr);
		}
	}

	OnSlotChanged.Broadcast();
}

bool ULxItemSlotData::SetItem(ULxItemBase* InItemData)
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

bool ULxItemSlotData::IsStack(ULxItemBase* InItemSlot)
{
	if (InItemSlot == nullptr || !InItemSlot->ItemIsValid() || ItemDataPtr == nullptr || !ItemDataPtr->ItemIsValid())
	{
		return false;
	}
	return ItemDataPtr->ItemIsStackable() && *ItemDataPtr ==  *InItemSlot;
}

bool ULxItemSlotData::CanAcceptItem(ULxItemBase* InItemData) const
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
		if (InItemData->ItemType() != ELxItemType::Equipment)
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
		ItemDataPtr->ItemStack(InItemSlot->ItemDataPtr);
		OnSlotChanged.Broadcast();
		const bool bSourceStillValid = InItemSlot->IsValid();
		if (bSourceStillValid)
		{
			InItemSlot->OnSlotChanged.Broadcast();
		}
		else
		{
			InItemSlot->ClearItem();
		}
		return bSourceStillValid ? ELxItemSlotDropResult::StackedPartial : ELxItemSlotDropResult::StackedAll;
	}

	ULxItemBase* TempItem = ItemDataPtr;
	SetItem(InItemSlot->ItemDataPtr);
	InItemSlot->SetItem(TempItem);
	return ELxItemSlotDropResult::Swapped;
}
/////////////////////////// ULxEquipmentSlotData //////////////////////////////////////
bool ULxEquipmentSlotData::CanAcceptItem(ULxItemBase* InItemData) const
{
	if (InItemData == nullptr || !InItemData->ItemIsValid())
	{
		return false;
	}
	if (InItemData->ItemType() != ELxItemType::Equipment)
	{
		return false;
	}
	ULxEquipment* EquipmentLogic = Cast<ULxEquipment>(InItemData);
	if (EquipmentLogic == nullptr)
	{
		return false;
	}
	if (EquipmentType != EquipmentLogic->EquipmentType())
	{
		return false;
	}
	return true;
}
/////////////////////////// ULxShortcutItemSlotData //////////////////////////////////////
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

	if (SourceSlot)
	{
		SourceSlot->OnSlotChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleSourceSlotChanged);
	}

	if (BoundItem)
	{
		BoundItem->OnItemCountChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
		BoundItem = nullptr;
	}

	SourceSlot = InSourceSlot;
	if (SourceSlot)
	{
		SourceSlot->OnSlotChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleSourceSlotChanged);
		SourceSlot->OnSlotChanged.AddDynamic(this, &ULxShortcutItemSlotData::HandleSourceSlotChanged);
	}

	HandleSourceSlotChanged();
	return true;
}

void ULxShortcutItemSlotData::UseItem()
{
	if (SourceSlot)
	{
		SourceSlot->UseItem();
		HandleSourceSlotChanged();
		return;
	}

	Super::UseItem();
}

bool ULxShortcutItemSlotData::CanAcceptItem(ULxItemBase* InItemData) const
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
	if (BoundItem)
	{
		BoundItem->OnItemCountChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
		BoundItem = nullptr;
	}

	BoundItem = (SourceSlot && SourceSlot->IsValid()) ? SourceSlot->ItemDataPtr : nullptr;
	ItemDataPtr = BoundItem;

	if (BoundItem)
	{
		BoundItem->OnItemCountChanged.RemoveDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
		BoundItem->OnItemCountChanged.AddDynamic(this, &ULxShortcutItemSlotData::HandleBoundItemChanged);
	}

	OnSlotChanged.Broadcast();
}

void ULxShortcutItemSlotData::HandleBoundItemChanged(ULxItemBase* Item, int32 OldCount, int32 NewCount)
{
	ItemDataPtr = (BoundItem && BoundItem->ItemIsValid()) ? BoundItem : nullptr;
	OnSlotChanged.Broadcast();
}
