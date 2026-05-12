#include "LxCharacterBackpackComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	struct FLxBackpackItemKey
	{
		ELxItemType ItemType = ELxItemType::None;
		FLxItemID ItemID = ItemIDNone;

		bool operator==(const FLxBackpackItemKey& Other) const
		{
			return ItemType == Other.ItemType && ItemID == Other.ItemID;
		}
	};

	uint32 GetTypeHash(const FLxBackpackItemKey& Key)
	{
		return HashCombine(::GetTypeHash(static_cast<uint8>(Key.ItemType)), ::GetTypeHash(Key.ItemID));
	}

	/** 查找第一个空背包槽位。 */
	ULxItemSlotData* FindEmptyBackpackSlot(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots)
	{
		for (ULxItemSlotData* Slot : InSlots)
		{
			if (Slot != nullptr && !Slot->IsValid())
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

		ULxItemBase* ExistingItem = Slot->GetItem();
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

		ULxItemBase* ExistingItem = Slot->GetItem();
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

bool ULxCharacterBackpackComponent::TestAddItemByTagID(FGameplayTag InItemIDTag, int32 InItemCount)
{
	const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(InItemIDTag);
	if (ItemConfig == nullptr)
	{
		return false;
	}

	return AddItemByRowID(ItemConfig->ItemType, ItemConfig->ItemID, InItemCount);
}

bool ULxCharacterBackpackComponent::CanAddItemList(const TArray<FLxItemQuote>& InItemList) const
{
	if (InItemList.IsEmpty())
	{
		return false;
	}

	TMap<FLxBackpackItemKey, int32> RequiredCountMap;
	TMap<FLxBackpackItemKey, int32> ExistingStackSpaceMap;
	TMap<FLxBackpackItemKey, int32> MaxStackCountMap;
	int32 EmptySlotCount = 0;

	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		const FLxBackpackItemKey ItemKey{ItemQuote.ItemType, ItemQuote.ItemID};
		if (ItemQuote.ItemType == ELxItemType::None || ItemQuote.ItemID == ItemIDNone || ItemQuote.ItemCount <= 0)
		{
			return false;
		}

		const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(ItemQuote.ItemType, ItemQuote.ItemID);
		if (ItemConfig == nullptr || ItemConfig->ItemCountMax <= 0)
		{
			return false;
		}

		RequiredCountMap.FindOrAdd(ItemKey) += ItemQuote.ItemCount;
		MaxStackCountMap.FindOrAdd(ItemKey) = FMath::Max(1, ItemConfig->ItemCountMax);
	}

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr || !Slot->IsValid())
		{
			++EmptySlotCount;
			continue;
		}

		ULxItemBase* ExistingItem = Slot->GetItem();
		if (ExistingItem == nullptr || !ExistingItem->ItemIsValid() || !ExistingItem->ItemIsStackable())
		{
			continue;
		}

		const FLxBackpackItemKey ItemKey{ExistingItem->ItemType(), ExistingItem->ItemID()};
		if (const int32* MaxStackCount = MaxStackCountMap.Find(ItemKey))
		{
			ExistingStackSpaceMap.FindOrAdd(ItemKey) += FMath::Max(0, *MaxStackCount - ExistingItem->ItemCount());
		}
	}

	int32 RequiredEmptySlotCount = 0;
	for (const TPair<FLxBackpackItemKey, int32>& RequiredPair : RequiredCountMap)
	{
		const int32 ExistingStackSpace = ExistingStackSpaceMap.FindRef(RequiredPair.Key);
		const int32 RemainingCount = FMath::Max(0, RequiredPair.Value - ExistingStackSpace);
		if (RemainingCount <= 0)
		{
			continue;
		}

		const int32 MaxStackCount = MaxStackCountMap.FindRef(RequiredPair.Key);
		RequiredEmptySlotCount += FMath::DivideAndRoundUp(RemainingCount, MaxStackCount);
	}

	return RequiredEmptySlotCount <= EmptySlotCount;
}

bool ULxCharacterBackpackComponent::AddItemList(const TArray<FLxItemQuote>& InItemList)
{
	if (!CanAddItemList(InItemList))
	{
		return false;
	}

	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		if (!AddItemByRowID(ItemQuote.ItemType, ItemQuote.ItemID, ItemQuote.ItemCount))
		{
			return false;
		}
	}
	return true;
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

		if (Slot == nullptr || !ItemMatches(Slot->GetItem(), InItemType, ItemID))
		{
			continue;
		}

		ULxItemBase* Item = Slot->GetItem();
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
		if (Slot != nullptr && Slot->GetItem() != nullptr && Slot->GetItem()->ItemType() == InItemType)
		{
			m_vFilteringCache.Add(Slot);
		}
	}
	return m_vFilteringCache;
}

void ULxCharacterBackpackComponent::NotifyItemUsedFromSlot(ULxItemBase* UsedItem)
{
	if (UsedItem == nullptr)
	{
		return;
	}

	OnItemUsed.Broadcast(UsedItem);
	OnDataChange.Broadcast();
}

void ULxCharacterBackpackComponent::HandleTrackedItemCountChanged(ULxItemBase* Item)
{
	NotifyItemUsedFromSlot(Item);
	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
}

void ULxCharacterBackpackComponent::HandleBackpackSlotChanged(ULxItemBase* InItemData)
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

		Slot->OnItemDataChanged.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleBackpackSlotChanged);
		Slot->OnItemDataChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleBackpackSlotChanged);

		if (Slot->GetItem() != nullptr)
		{
			Slot->GetItem()->OnItemCountChanged.RemoveDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemCountChanged);
			Slot->GetItem()->OnItemCountChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleTrackedItemCountChanged);
		}
	}
}

bool ULxCharacterBackpackComponent::CleanupInvalidItems()
{
	bool bChanged = false;

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot != nullptr && Slot->GetItem() != nullptr && !Slot->GetItem()->ItemIsValid())
		{
			Slot->ClearItem();
			bChanged = true;
		}
	}

	m_vItemList.Reset();
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot != nullptr && Slot->GetItem() != nullptr && Slot->GetItem()->ItemIsValid())
		{
			m_vItemList.AddUnique(Slot->GetItem());
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
		NewSlot->InitItemSlot(ELxItemSlotType::Backpack, LxTag_Item, nullptr);
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxCharacterBackpackComponent::HandleBackpackSlotChanged);
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
