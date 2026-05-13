#include "LxConsumable.h"

ULxConsumable::ULxConsumable()
{
	m_fConsumableInformation = FLxConsumableInformation();
}

ULxConsumable::~ULxConsumable()
{
}

ELxItemUseState ULxConsumable::ItemUse()
{
	if (ItemIsValid())
	{
		m_fConsumableInformation.ItemCount --;
		BroadcastItemCountChanged();
		return ELxItemUseState::ActivateEntry;
	}
	return ELxItemUseState::Failed;
}


FLxString ULxConsumable::ItemCountText()
{
	return m_fConsumableInformation.ItemCount > 1 ? FLxString(m_fConsumableInformation.ItemCount) : FLxString();
}

void ULxConsumable::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
	if (InItemData)
	{
		if (InItemData->ItemType == ELxItemType::Consumable)
		{
			m_fConsumableInformation = *static_cast<const FLxConsumableInformation*>(InItemData);
			m_fConsumableInformation.ItemCount = InItemCount;
		}
	}
}

FLxItemInformationBase* ULxConsumable::ItemBase()
{
	return &m_fConsumableInformation;
}
