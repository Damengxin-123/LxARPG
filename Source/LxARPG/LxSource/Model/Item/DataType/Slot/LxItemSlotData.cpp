#include "LxItemSlotData.h"

bool ULxItemSlotData::IsValid()
{
	return m_pItemData != nullptr && m_pItemData->ItemIsValid();
}

void ULxItemSlotData::ItemUse()
{
	if (!IsValid())
	{
		return;
	}

	ULxItemBase* UsedItem = m_pItemData;
	const FLxItemCount OldItemCount = UsedItem->ItemCount();
	const ELxItemUseState UseState = UsedItem->ItemUse();
	if (UseState == ELxItemUseState::Failed)
	{
		return;
	}
	//
	// if (OldItemCount != UsedItem->ItemCount())
	// {
	// 	UsedItem->BroadcastItemCountChanged();
	// }
}

void ULxItemSlotData::InitItemSlot(ELxItemSlotType InItemSlotType, FGameplayTag InItemType, ULxItemBase* InItemData)
{
	if (InItemSlotType == ELxItemSlotType::None)
	{
		return;
	}

	m_eSlotType = InItemSlotType;
	m_fSlotLogicSet = ConstItemSlotLogicSetMap[InItemSlotType];
	m_fItemTypeTag = InItemType;
	SetItem(InItemData);
}

bool ULxItemSlotData::SetItem(ULxItemBase* InItemData)
{
	if (InItemData == nullptr || !InItemData->ItemIsValid())
	{
		return false;
	}

	if (m_pItemData)
	{
		m_pItemData->OnItemCountChanged.RemoveDynamic(this, &ULxItemSlotData::HandleItemCountChanged);
	}

	m_pItemData = InItemData;
	m_pItemData->OnItemCountChanged.RemoveDynamic(this, &ULxItemSlotData::HandleItemCountChanged);
	m_pItemData->OnItemCountChanged.AddDynamic(this, &ULxItemSlotData::HandleItemCountChanged);
	OnItemDataChanged.Broadcast(m_pItemData);
	return true;
}

void ULxItemSlotData::ClearItem()
{
	if (m_pItemData)
	{
		m_pItemData->OnItemCountChanged.RemoveDynamic(this, &ULxItemSlotData::HandleItemCountChanged);
	}

	m_pItemData = nullptr;
	OnItemDataChanged.Broadcast(m_pItemData);
}

void ULxItemSlotData::HandleItemCountChanged(ULxItemBase* ChangedItem)
{
	if (ChangedItem != m_pItemData)
	{
		return;
	}

	OnItemDataChanged.Broadcast(m_pItemData);
}

bool ULxItemSlotData::ItemIsEnter() const
{
	return m_fSlotLogicSet.ItemEnter;
}

bool ULxItemSlotData::ItemIsLeave() const
{
	return m_fSlotLogicSet.ItemLeave;
}

ELxItemSlotDropResult ULxItemSlotData::ItemEnterToThis(ULxItemSlotData* InItemSlot)
{
	if (InItemSlot == nullptr || !InItemSlot->IsValid())
	{
		return ELxItemSlotDropResult::FailedInvalidSource;
	}

	if (!ItemIsEnter() || !InItemSlot->ItemIsLeave())
	{
		return ELxItemSlotDropResult::CannotEnter;
	}

	if (!InItemSlot->m_pItemData->ItemIDTag().MatchesTag(m_fItemTypeTag))
	{
		return ELxItemSlotDropResult::TypeError;
	}

	if (m_fSlotLogicSet.ItemIsShortcut)
	{
		SetItem(InItemSlot->m_pItemData);
		return ELxItemSlotDropResult::EnterSuccess;
	}

	if (!IsValid())
	{
		SetItem(InItemSlot->m_pItemData);
		InItemSlot->ClearItem();
		return ELxItemSlotDropResult::EnterSuccess;
	}

	if (m_pItemData->ItemIsStackable() && m_pItemData->ItemIDTag() == InItemSlot->m_pItemData->ItemIDTag())
	{
		m_pItemData->ItemStack(InItemSlot->m_pItemData);
		OnItemDataChanged.Broadcast(m_pItemData);

		if (InItemSlot->IsValid())
		{
			InItemSlot->OnItemDataChanged.Broadcast(InItemSlot->m_pItemData);
			return ELxItemSlotDropResult::StackedPartial;
		}

		InItemSlot->ClearItem();
		return ELxItemSlotDropResult::StackedAll;
	}

	ULxItemBase* TempItemData = m_pItemData;
	SetItem(InItemSlot->m_pItemData);
	InItemSlot->SetItem(TempItemData);
	return ELxItemSlotDropResult::Swapped;
}
