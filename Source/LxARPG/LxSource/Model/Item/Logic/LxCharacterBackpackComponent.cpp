#include "LxCharacterBackpackComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Algo/Sort.h"
#include "GameFramework/Actor.h"
#include "Misc/Crc.h"
#include "Net/UnrealNetwork.h"

namespace
{
	struct FLxBackpackItemKey
	{
		FGameplayTag ItemIDTag;

		bool operator==(const FLxBackpackItemKey& Other) const
		{
			return ItemIDTag == Other.ItemIDTag;
		}
	};

	uint32 GetTypeHash(const FLxBackpackItemKey& Key)
	{
		return FCrc::StrCrc32(*Key.ItemIDTag.ToString());
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

	/** 判断物品对象是否匹配指定标签 ID。 */
	bool ItemMatches(ULxItemBase* InItem, FGameplayTag InItemIDTag)
	{
		return InItem != nullptr
			&& InItem->ItemIsValid()
			&& InItem->ItemIDTag() == InItemIDTag;
	}
}

ULxCharacterBackpackModule::ULxCharacterBackpackModule()
{
}

void ULxCharacterBackpackModule::OnModuleInitialize()
{
	m_pOwnerCharacter = GetCharacterOwner();
	InitializeBackpack();
}

void ULxCharacterBackpackModule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxCharacterBackpackModule, ReplicatedBackpackSlots);
}

bool ULxCharacterBackpackModule::AddItemByTagID(FGameplayTag InItemIDTag, int32 InItemCount)
{
	if (!InItemIDTag.IsValid() || InItemCount <= 0)
	{
		return false;
	}

	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		if (ULxCharacterContentComponent* OwnerContentComponent = GetContentComponent())
		{
			OwnerContentComponent->ServerAddBackpackItems({FLxItemQuote(InItemIDTag, InItemCount)});
		}
		return true;
	}

	const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(InItemIDTag);
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
		if (ItemMatches(ExistingItem, InItemIDTag) && ExistingItem->ItemIsStackable())
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
		if (!ItemMatches(ExistingItem, InItemIDTag) || !ExistingItem->ItemIsStackable())
		{
			continue;
		}

		const int32 StackableCount = FMath::Min(RemainingCount, MaxStackCount - ExistingItem->ItemCount());
		if (StackableCount <= 0)
		{
			continue;
		}

		ULxItemBase* IncomingStack = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemIDTag, static_cast<FLxItemCount>(StackableCount)));
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
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemIDTag, static_cast<FLxItemCount>(NewStackCount)));
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
	SyncReplicatedBackpackSlots();
	return RemainingCount <= 0;
}

bool ULxCharacterBackpackModule::CanAddItemList(const TArray<FLxItemQuote>& InItemList) const
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
		const FLxBackpackItemKey ItemKey{ItemQuote.ItemIDTag};
		if (!ItemQuote.ItemIDTag.IsValid() || ItemQuote.ItemCount <= 0)
		{
			return false;
		}

		const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(ItemQuote.ItemIDTag);
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

		const FLxBackpackItemKey ItemKey{ExistingItem->ItemIDTag()};
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

bool ULxCharacterBackpackModule::AddItemList(const TArray<FLxItemQuote>& InItemList)
{
	if (!CanAddItemList(InItemList))
	{
		return false;
	}

	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		if (!AddItemByTagID(ItemQuote.ItemIDTag, ItemQuote.ItemCount))
		{
			return false;
		}
	}
	return true;
}

bool ULxCharacterBackpackModule::CheckHaveItemList(const TArray<FLxItemQuote>& InItemList) const
{
	if (InItemList.IsEmpty())
	{
		return false;
	}

	TMap<FLxBackpackItemKey, int32> RequiredCountMap;
	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		if (!ItemQuote.ItemIDTag.IsValid() || ItemQuote.ItemCount <= 0)
		{
			return false;
		}

		RequiredCountMap.FindOrAdd(FLxBackpackItemKey{ItemQuote.ItemIDTag}) += ItemQuote.ItemCount;
	}

	for (const TPair<FLxBackpackItemKey, int32>& RequiredPair : RequiredCountMap)
	{
		if (!CheckHaveItem(RequiredPair.Key.ItemIDTag, RequiredPair.Value))
		{
			return false;
		}
	}
	return true;
}

bool ULxCharacterBackpackModule::RemoveItemList(const TArray<FLxItemQuote>& InItemList)
{
	if (!CheckHaveItemList(InItemList))
	{
		return false;
	}

	TMap<FLxBackpackItemKey, int32> RequiredCountMap;
	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		RequiredCountMap.FindOrAdd(FLxBackpackItemKey{ItemQuote.ItemIDTag}) += ItemQuote.ItemCount;
	}

	for (const TPair<FLxBackpackItemKey, int32>& RequiredPair : RequiredCountMap)
	{
		if (!RemoveItemAt(RequiredPair.Key.ItemIDTag, RequiredPair.Value))
		{
			return false;
		}
	}
	return true;
}

bool ULxCharacterBackpackModule::RemoveItemAt(FGameplayTag InItemIDTag, int32 InItemCount)
{
	if (!CheckHaveItem(InItemIDTag, InItemCount))
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

		if (Slot == nullptr || !ItemMatches(Slot->GetItem(), InItemIDTag))
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
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemIDTag, static_cast<FLxItemCount>(NewCount)));
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
	SyncReplicatedBackpackSlots();
	return true;
}

bool ULxCharacterBackpackModule::CheckHaveItem(FGameplayTag InItemIDTag, int32 InItemCount) const
{
	if (!InItemIDTag.IsValid() || InItemCount <= 0)
	{
		return false;
	}

	int32 FoundCount = 0;
	for (ULxItemBase* Item : m_vItemList)
	{
		if (ItemMatches(Item, InItemIDTag))
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

void ULxCharacterBackpackModule::SortingOfItems()
{
	CleanupInvalidItems();

	Algo::Sort(m_vItemList, [](const TObjectPtr<ULxItemBase>& Left, const TObjectPtr<ULxItemBase>& Right)
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
	SyncReplicatedBackpackSlots();
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackModule::GetAllItems()
{
	return m_vBackpackSlots;
}

ULxItemSlotData* ULxCharacterBackpackModule::GetBackpackSlotAt(int32 SlotIndex) const
{
	return m_vBackpackSlots.IsValidIndex(SlotIndex) ? m_vBackpackSlots[SlotIndex] : nullptr;
}

bool ULxCharacterBackpackModule::MoveBackpackSlot(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	ULxItemSlotData* SourceSlot = GetBackpackSlotAt(SourceSlotIndex);
	ULxItemSlotData* TargetSlot = GetBackpackSlotAt(TargetSlotIndex);
	if (!SourceSlot || !TargetSlot || SourceSlot == TargetSlot)
	{
		return false;
	}

	const ELxItemSlotDropResult DropResult = TargetSlot->ItemEnterToThis(SourceSlot);
	const bool bSucceeded = DropResult == ELxItemSlotDropResult::Swapped
		|| DropResult == ELxItemSlotDropResult::StackedAll
		|| DropResult == ELxItemSlotDropResult::StackedPartial
		|| DropResult == ELxItemSlotDropResult::EnterSuccess;
	if (!bSucceeded)
	{
		return false;
	}

	SyncReplicatedBackpackSlots();
	return true;
}

void ULxCharacterBackpackModule::SyncReplicatedBackpackSlots()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedBackpackSlots.Reset();
	ReplicatedBackpackSlots.Reserve(m_vBackpackSlots.Num());
	for (ULxItemSlotData* SlotData : m_vBackpackSlots)
	{
		ReplicatedBackpackSlots.Add(BuildItemQuoteFromSlot(SlotData));
	}
}

void ULxCharacterBackpackModule::NotifyItemUsedFromSlot(ULxItemBase* UsedItem)
{
	if (UsedItem == nullptr)
	{
		return;
	}

	OnItemUsed.Broadcast(UsedItem);
	OnDataChange.Broadcast();
}

void ULxCharacterBackpackModule::HandleTrackedItemCountChanged(ULxItemBase* Item)
{
	NotifyItemUsedFromSlot(Item);
	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	SyncReplicatedBackpackSlots();
}

void ULxCharacterBackpackModule::HandleBackpackSlotChanged(ULxItemBase* InItemData)
{
	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	SyncReplicatedBackpackSlots();
}

void ULxCharacterBackpackModule::RefreshTrackedBindings()
{
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr)
		{
			continue;
		}

		Slot->OnItemDataChanged.RemoveDynamic(this, &ULxCharacterBackpackModule::HandleBackpackSlotChanged);
		Slot->OnItemDataChanged.AddDynamic(this, &ULxCharacterBackpackModule::HandleBackpackSlotChanged);

		if (Slot->GetItem() != nullptr)
		{
			Slot->GetItem()->OnItemCountChanged.RemoveDynamic(this, &ULxCharacterBackpackModule::HandleTrackedItemCountChanged);
			Slot->GetItem()->OnItemCountChanged.AddDynamic(this, &ULxCharacterBackpackModule::HandleTrackedItemCountChanged);
		}
	}
}

bool ULxCharacterBackpackModule::CleanupInvalidItems()
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

void ULxCharacterBackpackModule::InitializeBackpack()
{
	m_vBackpackSlots.Empty();
	m_vItemList.Empty();

	for (int32 Index = 0; Index < BackpackSlotCount; ++Index)
	{
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->SetSlotIndex(Index);
		NewSlot->InitItemSlot(ELxItemSlotType::Backpack, LxTag_Item, nullptr);
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxCharacterBackpackModule::HandleBackpackSlotChanged);
		m_vBackpackSlots.Add(NewSlot);
	}
}

void ULxCharacterBackpackModule::ApplyReplicatedBackpackSlots()
{
	const int32 DesiredSlotCount = FMath::Max(BackpackSlotCount, ReplicatedBackpackSlots.Num());
	if (m_vBackpackSlots.Num() != DesiredSlotCount)
	{
		BackpackSlotCount = DesiredSlotCount;
		InitializeBackpack();
	}

	for (int32 Index = 0; Index < m_vBackpackSlots.Num(); ++Index)
	{
		ULxItemSlotData* SlotData = m_vBackpackSlots[Index];
		if (!SlotData)
		{
			continue;
		}

		SlotData->SetSlotIndex(Index);
		if (!ReplicatedBackpackSlots.IsValidIndex(Index)
			|| !ReplicatedBackpackSlots[Index].ItemIDTag.IsValid()
			|| ReplicatedBackpackSlots[Index].ItemCount <= 0)
		{
			SlotData->ClearItem();
			continue;
		}

		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ReplicatedBackpackSlots[Index]);
		if (NewItem)
		{
			SlotData->SetItem(NewItem);
		}
		else
		{
			SlotData->ClearItem();
		}
	}

	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
}

FLxItemQuote ULxCharacterBackpackModule::BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const
{
	if (!SlotData || !SlotData->IsValid() || !SlotData->GetItem())
	{
		return FLxItemQuote();
	}

	return FLxItemQuote(SlotData->GetItem()->ItemIDTag(), SlotData->GetItem()->ItemCount());
}

void ULxCharacterBackpackModule::OnRep_BackpackSlots()
{
	ApplyReplicatedBackpackSlots();
}
