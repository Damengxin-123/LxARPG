#include "LxItemSlotData.h"

bool ULxItemSlotData::IsValid()
{
	return m_pItemData != nullptr && m_pItemData->ItemIsValid();
}

void ULxItemSlotData::ItemUse()
{
	if (!IsValid()
		|| m_eSlotType == ELxItemSlotType::TreasureChest
		|| m_eSlotType == ELxItemSlotType::Transaction)
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

int32 ULxItemSlotData::GetItemValue() const
{
	if (m_pItemData == nullptr || !m_pItemData->ItemIsValid())
	{
		return 0;
	}

	const FLxItemInformationBase ItemInformation = m_pItemData->ItemInformation();
	const int32 SingleValue = FMath::Max(0, ItemInformation.ItemSellPrice);
	const int64 BaseValue = static_cast<int64>(SingleValue) * static_cast<int64>(m_pItemData->ItemCount());
	const double AdjustedValue = static_cast<double>(BaseValue) * static_cast<double>(FMath::Max(0.0f, ItemValueRate));
	if (AdjustedValue <= 0.0)
	{
		return 0;
	}

	const int64 RoundedValue = static_cast<int64>(AdjustedValue + 0.5);
	return RoundedValue > MAX_int32 ? MAX_int32 : static_cast<int32>(RoundedValue);
}

void ULxItemSlotData::SetItemValueRate(float InItemValueRate)
{
	const float NewItemValueRate = FMath::Max(0.0f, InItemValueRate);
	if (FMath::IsNearlyEqual(ItemValueRate, NewItemValueRate))
	{
		return;
	}

	ItemValueRate = NewItemValueRate;
	OnItemDataChanged.Broadcast(m_pItemData);
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
	if (m_eSlotType == ELxItemSlotType::Transaction && !bCanTrade)
	{
		return false;
	}

	return m_fSlotLogicSet.ItemLeave;
}

void ULxItemSlotData::SetCanTrade(bool bInCanTrade)
{
	if (bCanTrade == bInCanTrade)
	{
		return;
	}

	bCanTrade = bInCanTrade;
	OnItemDataChanged.Broadcast(m_pItemData);
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

	if (m_eSlotType == ELxItemSlotType::Transaction || InItemSlot->m_eSlotType == ELxItemSlotType::Transaction)
	{
		return ELxItemSlotDropResult::CannotEnter;
	}

	if (m_eSlotType == ELxItemSlotType::TreasureChest && InItemSlot->m_eSlotType == ELxItemSlotType::TreasureChest)
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
