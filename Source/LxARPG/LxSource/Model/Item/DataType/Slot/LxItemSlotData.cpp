#include "LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"

bool ULxItemSlotData::IsValid()
{
	return m_pItemData != nullptr && m_pItemData->ItemIsValid();
}

void ULxItemSlotData::ItemUse()
{
	if (LxSafeItemOperations::IsSlotBusy(this) || !IsValid()
		|| m_eSlotType == ELxItemSlotType::TreasureChest
		|| m_eSlotType == ELxItemSlotType::Transaction)
	{
		return;
	}

	// 使用期间可能清空槽位并解绑监听，保留使用前的事件副本以确保最后一个物品仍触发效果。
	ULxItemBase* UsedItem = m_pItemData;
	const FOnLxItemUseSucceeded UseListeners = UsedItem->OnItemUseSucceeded;
	if (UsedItem->ItemUse() == ELxItemUseState::ActivateEntry)
	{
		UseListeners.Broadcast(UsedItem);
	}
}

void ULxItemSlotData::StartUseItem()
{
	if (LxSafeItemOperations::IsSlotBusy(this) || !IsValid()
		|| m_eSlotType == ELxItemSlotType::TreasureChest
		|| m_eSlotType == ELxItemSlotType::Transaction)
	{
		return;
	}

	ULxItemBase* UsedItem = m_pItemData;
	const FOnLxItemUseSucceeded UseListeners = UsedItem->OnItemUseSucceeded;
	if (UsedItem->ItemUseStart() == ELxItemUseState::ActivateEntry)
	{
		UseListeners.Broadcast(UsedItem);
	}
}

void ULxItemSlotData::EndUseItem()
{
	if (LxSafeItemOperations::IsSlotBusy(this) || !IsValid()
		|| m_eSlotType == ELxItemSlotType::TreasureChest
		|| m_eSlotType == ELxItemSlotType::Transaction)
	{
		return;
	}

	ULxItemBase* UsedItem = m_pItemData;
	const FOnLxItemUseSucceeded UseListeners = UsedItem->OnItemUseSucceeded;
	if (UsedItem->ItemUseEnd() == ELxItemUseState::ActivateEntry)
	{
		UseListeners.Broadcast(UsedItem);
	}
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

bool ULxItemSlotData::SetItem(ULxItemBase* InItemData, bool bBroadcast)
{
	// 安全事务内部使用静默提交；常规入口不能在事务通知回调中覆盖槽位。
	if (bBroadcast && LxSafeItemOperations::IsSlotBusy(this)) return false;
	if (InItemData == nullptr || !InItemData->ItemIsValid())
	{
		return false;
	}

	if (m_pItemData)
	{
		m_pItemData->OnItemCountChanged.RemoveAll(this);
	}

	m_pItemData = InItemData;
	m_pItemData->OnItemCountChanged.RemoveAll(this);
	m_pItemData->OnItemCountChanged.AddUObject(this, &ULxItemSlotData::HandleItemCountChanged);
	if (bBroadcast)
	{
		NotifyItemDataChanged();
	}
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

void ULxItemSlotData::ClearItem(bool bBroadcast)
{
	if (bBroadcast && LxSafeItemOperations::IsSlotBusy(this)) return;
	if (m_pItemData)
	{
		m_pItemData->OnItemCountChanged.RemoveAll(this);
	}

	m_pItemData = nullptr;
	if (bBroadcast)
	{
		NotifyItemDataChanged();
	}
}

void ULxItemSlotData::NotifyItemDataChanged()
{
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
	// 商城商品始终允许发起拖拽；购买资格在放下时由权威交易逻辑再次校验。
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
	if (LxSafeItemOperations::IsSlotBusy(this) || LxSafeItemOperations::IsSlotBusy(InItemSlot))
	{
		return ELxItemSlotDropResult::CannotEnter;
	}
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

	// 技能展示槽只负责显示技能和向快捷栏拖出，不接收任何槽位拖入。
	if (m_eSlotType == ELxItemSlotType::SkillDisplay)
	{
		return ELxItemSlotDropResult::CannotEnter;
	}

	if (m_eSlotType == ELxItemSlotType::TreasureChest && InItemSlot->m_eSlotType == ELxItemSlotType::TreasureChest)
	{
		return ELxItemSlotDropResult::CannotEnter;
	}

	// 技能物品只能进入快捷栏形成快捷引用，不能被放入背包、仓库、装备等普通槽位。
	if (InItemSlot->m_pItemData->ItemType() == ELxItemType::Skill && m_eSlotType != ELxItemSlotType::Shortcut)
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
