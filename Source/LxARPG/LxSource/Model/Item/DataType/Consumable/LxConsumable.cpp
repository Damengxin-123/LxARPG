#include "LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"

ULxConsumable::ULxConsumable()
{
	m_fConsumableInformation = FLxConsumableInformation();
}

ULxConsumable::~ULxConsumable()
{
}

ELxItemUseState ULxConsumable::ItemUse()
{
	// 直接调用物品对象也不能在交易或存取最终通知中消耗仍处于事务锁内的实例。
	if (LxSafeItemOperations::IsContainerBusy(GetOuter())) return ELxItemUseState::Failed;
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
