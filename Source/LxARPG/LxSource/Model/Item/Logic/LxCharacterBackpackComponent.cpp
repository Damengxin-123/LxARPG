#include "LxCharacterBackpackComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	/** 查找第一个空背包槽位。 */
	ULxItemSlotData* FindEmptyBackpackSlot(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots)
	{
		for (ULxItemSlotData* Slot : InSlots)
		{
			if (Slot != nullptr && Slot->ItemDataPtr == nullptr)
			{
				return Slot;
			}
		}
		return nullptr;
	}

	/** 判断物品对象是否匹配指定类型和 ID。 */
	bool ItemMatches(ULxItemBase* InItem, ELxItemType InItemType, FLxItemID InItemID)
	{
		return InItem != nullptr
			&& InItem->ItemIsValid()
			&& InItem->ItemType() == InItemType
			&& InItem->ItemID() == InItemID;
	}
}

ULxCharacterBackpackComponent::ULxCharacterBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterBackpackComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	m_pOwnerCharacter = GetCharacterOwner();
	InitializeBackpack();
}

bool ULxCharacterBackpackComponent::AddItemByRowID(ELxItemType InItemType, int32 InItemID, int32 InItemCount)
{
	const FLxItemID ItemID = InItemID;
	if (InItemType == ELxItemType::None || ItemID == ItemIDNone || InItemCount <= 0)
	{
		return false;
	}

	const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(InItemType, ItemID);
	if (ItemConfig == nullptr || ItemConfig->ItemCountMax <= 0)
	{
		return false;
	}

	if (m_vBackpackSlots.IsEmpty())
	{
		InitializeBackpack();
	}

	CleanupInvalidItems();

	const int32 MaxStackCount = FMath::Max(1, ItemConfig->ItemCountMax);
	int32 AvailableCount = 0;
	int32 EmptySlotCount = 0;

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr)
		{
			continue;
		}

		if (!Slot->IsValid())
		{
			++EmptySlotCount;
			continue;
		}

		ULxItemBase* ExistingItem = Slot->ItemDataPtr;
		if (ItemMatches(ExistingItem, InItemType, ItemID) && ExistingItem->ItemIsStackable())
		{
			AvailableCount += FMath::Max(0, MaxStackCount - ExistingItem->ItemCount());
		}
	}

	AvailableCount += EmptySlotCount * MaxStackCount;
	if (AvailableCount < InItemCount)
	{
		return false;
	}

	int32 RemainingCount = InItemCount;

	// 先把新增数量堆叠到已有背包槽位中的同类物品里，保持数量变化事件从物品对象发出。
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (RemainingCount <= 0)
		{
			break;
		}

		if (Slot == nullptr || !Slot->IsValid())
		{
			continue;
		}

		ULxItemBase* ExistingItem = Slot->ItemDataPtr;
		if (!ItemMatches(ExistingItem, InItemType, ItemID) || !ExistingItem->ItemIsStackable())
		{
			continue;
		}

		const int32 StackableCount = FMath::Min(RemainingCount, MaxStackCount - ExistingItem->ItemCount());
		if (StackableCount <= 0)
		{
			continue;
		}

		ULxItemBase* IncomingStack = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemType, ItemID, static_cast<FLxItemCount>(StackableCount)));
		if (IncomingStack == nullptr || !IncomingStack->ItemIsValid())
		{
			return false;
		}

		ExistingItem->ItemStack(IncomingStack);
		RemainingCount -= StackableCount - IncomingStack->ItemCount();
	}

	// 剩余数量按最大堆叠数拆分为多个物品对象放入空槽位。
	while (RemainingCount > 0)
	{
		ULxItemSlotData* EmptySlot = FindEmptyBackpackSlot(m_vBackpackSlots);
		if (EmptySlot == nullptr)
		{
			break;
		}

		const int32 NewStackCount = FMath::Min(RemainingCount, MaxStackCount);
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemType, ItemID, static_cast<FLxItemCount>(NewStackCount)));
		if (NewItem == nullptr || !NewItem->ItemIsValid())
		{
			return false;
		}

		if (!EmptySlot->SetItem(NewItem))
		{
			return false;
		}

		RemainingCount -= NewStackCount;
	}

	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	return RemainingCount <= 0;
}

bool ULxCharacterBackpackComponent::RemoveItemAt(ELxItemType InItemType, FName InItemID, int32 InItemCount)
{
	FLxItemID ItemID = ResolveItemIDFromName(InItemID);
	if (!CheckHaveItem(InItemType, InItemID, InItemCount))
	{
		return false;
	}

	int32 RemainingRemoveCount = InItemCount;
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (RemainingRemoveCount <= 0)
		{
			break;
		}

		if (Slot == nullptr || !ItemMatches(Slot->ItemDataPtr, InItemType, ItemID))
		{
			continue;
		}

		ULxItemBase* Item = Slot->ItemDataPtr;
		const int32 ItemCount = Item->ItemCount();
		if (ItemCount <= RemainingRemoveCount)
		{
			RemainingRemoveCount -= ItemCount;
			Slot->ClearItem();
			m_vItemList.Remove(Item);
			continue;
		}

		const int32 NewCount = ItemCount - RemainingRemoveCount;
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemType, ItemID, static_cast<FLxItemCount>(NewCount)));
		if (NewItem != nullptr && NewItem->ItemIsValid())
		{
			m_vItemList.Remove(Item);
			m_vItemList.Add(NewItem);
			Slot->SetItem(NewItem);
		}
		RemainingRemoveCount = 0;
	}

	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	return true;
}

bool ULxCharacterBackpackComponent::CheckHaveItem(ELxItemType InItemType, FName InItemID, int32 InItemCount) const
{
	const FLxItemID ItemID = ResolveItemIDFromName(InItemID);
	if (ItemID == ItemIDNone || InItemCount <= 0)
	{
		return false;
	}

	int32 FoundCount = 0;
	for (ULxItemBase* Item : m_vItemList)
	{
		if (ItemMatches(Item, InItemType, ItemID))
		{
			FoundCount += Item->ItemCount();
			if (FoundCount >= InItemCount)
			{
				return true;
			}
		}
	}
	return false;
}

void ULxCharacterBackpackComponent::SortingOfItems()
{
	CleanupInvalidItems();

	m_vItemList.Sort([](const TObjectPtr<ULxItemBase>& Left, const TObjectPtr<ULxItemBase>& Right)
	{
		if (Left == nullptr || Right == nullptr)
		{
			return Left != nullptr;
		}
		return *Left < *Right;
	});

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot)
		{
			Slot->ClearItem();
		}
	}

	for (int32 Index = 0; Index < m_vItemList.Num() && Index < m_vBackpackSlots.Num(); ++Index)
	{
		if (m_vBackpackSlots[Index])
		{
			m_vBackpackSlots[Index]->SetItem(m_vItemList[Index]);
		}
	}

	RefreshTrackedBindings();
	OnDataChange.Broadcast();
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackComponent::GetAllItems()
{
	return m_vBackpackSlots;
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackComponent::QueryItemsOnItemType(ELxItemType InItemType)
{
	m_vFilteringCache.Empty();
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot != nullptr && Slot->ItemDataPtr != nullptr && Slot->ItemDataPtr->ItemType() == InItemType)
		{
			m_vFilteringCache.Add(Slot);
		}
	}
	return m_vFilteringCache;
}

void ULxCharacterBackpackComponent::NotifyItemUsedFromSlot(ULxItemBase* UsedItem)
{
	if (UsedItem == nullptr || !UsedItem->ItemIsValid())
	{
		return;
	}

	OnItemUsed.Broadcast(UsedItem);
	OnDataChange.Broadcast();
}

void ULxCharacterBackpackComponent::HandleTrackedItemCountChanged(ULxItemBase* Item, int32 OldCount, int32 NewCount)
{
	CleanupInvalidItems();
	OnDataChange.Broadcast();
}

void ULxCharacterBackpackComponent::HandleBackpackSlotChanged()
{
	CleanupInvalidItems();
	OnDataChange.Broadcast();
}

void ULxCharacterBackpackComponent::RefreshTrackedBindings()
{
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr)
		{
			continue;
		}

		Slot->OnSlotChanged.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleBackpackSlotChanged);
		Slot->OnSlotChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleBackpackSlotChanged);

		if (Slot->ItemDataPtr != nullptr)
		{
			Slot->ItemDataPtr->OnItemCountChanged.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemCountChanged);
			Slot->ItemDataPtr->OnItemCountChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemCountChanged);
		}
	}
}

bool ULxCharacterBackpackComponent::CleanupInvalidItems()
{
	bool bChanged = false;

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot != nullptr && Slot->ItemDataPtr != nullptr && !Slot->ItemDataPtr->ItemIsValid())
		{
			Slot->ClearItem();
			bChanged = true;
		}
	}

	m_vItemList.Reset();
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot != nullptr && Slot->ItemDataPtr != nullptr && Slot->ItemDataPtr->ItemIsValid())
		{
			m_vItemList.AddUnique(Slot->ItemDataPtr);
		}
	}

	return bChanged;
}

void ULxCharacterBackpackComponent::InitializeBackpack()
{
	m_vFilteringCache.Empty();
	m_vBackpackSlots.Empty();
	m_vItemList.Empty();

	for (int32 Index = 0; Index < BackpackSlotCount; ++Index)
	{
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->ItemSlotType = ELxItemSlotType::Backpack;
		NewSlot->ID = Index;
		NewSlot->OnSlotChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleBackpackSlotChanged);
		m_vBackpackSlots.Add(NewSlot);
	}
}

FLxItemID ULxCharacterBackpackComponent::ResolveItemIDFromName(FName InItemID)
{
	if (InItemID.IsNone())
	{
		return ItemIDNone;
	}

	const FString ItemIDString = InItemID.ToString();
	return static_cast<FLxItemID>(FCString::Strtoui64(*ItemIDString, nullptr, 0));
}
