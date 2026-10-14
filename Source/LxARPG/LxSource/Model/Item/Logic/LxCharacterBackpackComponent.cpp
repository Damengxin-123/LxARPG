#include "LxCharacterBackpackComponent.h"
#include "LxSafeItemOperations.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "Algo/Sort.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
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

void ULxCharacterBackpackModule::OnModuleInitialize()
{
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

	const int32 MaxStackCount = ItemConfig->ItemCountMax;
	// 多个32位堆叠的容量合计使用64位，避免高堆叠上限使容量检查溢出。
	int64 AvailableCount = 0;
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

	AvailableCount += static_cast<int64>(EmptySlotCount) * MaxStackCount;
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

		ULxItemBase* IncomingStack = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemIDTag, StackableCount));
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
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemIDTag, NewStackCount));
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

	TMap<FGameplayTag, int64> RequiredCountMap;
	TMap<FGameplayTag, int64> ExistingStackSpaceMap;
	TMap<FGameplayTag, int32> MaxStackCountMap;
	int32 EmptySlotCount = 0;

	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		if (!ItemQuote.ItemIDTag.IsValid() || ItemQuote.ItemCount <= 0)
		{
			return false;
		}

		const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(ItemQuote.ItemIDTag);
		if (ItemConfig == nullptr || ItemConfig->ItemCountMax <= 0)
		{
			return false;
		}

		RequiredCountMap.FindOrAdd(ItemQuote.ItemIDTag) += ItemQuote.ItemCount;
		MaxStackCountMap.FindOrAdd(ItemQuote.ItemIDTag) = ItemConfig->ItemCountMax;
	}

	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr || !Slot->IsValid())
		{
			++EmptySlotCount;
			continue;
		}

		ULxItemBase* ExistingItem = Slot->GetItem();
		if (!ExistingItem->ItemIsStackable())
		{
			continue;
		}

		const FGameplayTag ItemKey = ExistingItem->ItemIDTag();
		if (const int32* MaxStackCount = MaxStackCountMap.Find(ItemKey))
		{
			ExistingStackSpaceMap.FindOrAdd(ItemKey) += FMath::Max(0, *MaxStackCount - ExistingItem->ItemCount());
		}
	}

	int64 RequiredEmptySlotCount = 0;
	for (const TPair<FGameplayTag, int64>& RequiredPair : RequiredCountMap)
	{
		const int64 ExistingStackSpace = ExistingStackSpaceMap.FindRef(RequiredPair.Key);
		const int64 RemainingCount = FMath::Max<int64>(0, RequiredPair.Value - ExistingStackSpace);
		if (RemainingCount <= 0)
		{
			continue;
		}

		const int32 MaxStackCount = MaxStackCountMap.FindRef(RequiredPair.Key);
		RequiredEmptySlotCount += FMath::DivideAndRoundUp(RemainingCount, static_cast<int64>(MaxStackCount));
	}

	return RequiredEmptySlotCount <= EmptySlotCount;
}

bool ULxCharacterBackpackModule::AddItemList(const TArray<FLxItemQuote>& InItemList)
{
	if (LxSafeItemOperations::IsContainerBusy(this)) return false;
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

	// 同类物品可分布在多个堆叠，批次总需求不能缩回单堆的32位范围。
	TMap<FGameplayTag, int64> RequiredCountMap;
	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		if (!ItemQuote.ItemIDTag.IsValid() || ItemQuote.ItemCount <= 0)
		{
			return false;
		}

		RequiredCountMap.FindOrAdd(ItemQuote.ItemIDTag) += ItemQuote.ItemCount;
	}

	for (const TPair<FGameplayTag, int64>& RequiredPair : RequiredCountMap)
	{
		if (!CheckHaveItem(RequiredPair.Key, RequiredPair.Value))
		{
			return false;
		}
	}
	return true;
}

bool ULxCharacterBackpackModule::RemoveItemList(const TArray<FLxItemQuote>& InItemList)
{
	if (LxSafeItemOperations::IsContainerBusy(this)) return false;
	if (!CheckHaveItemList(InItemList))
	{
		return false;
	}

	TMap<FGameplayTag, int64> RequiredCountMap;
	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		RequiredCountMap.FindOrAdd(ItemQuote.ItemIDTag) += ItemQuote.ItemCount;
	}

	for (const TPair<FGameplayTag, int64>& RequiredPair : RequiredCountMap)
	{
		if (!RemoveItemAt(RequiredPair.Key, RequiredPair.Value))
		{
			return false;
		}
	}
	return true;
}

bool ULxCharacterBackpackModule::RemoveItemAt(FGameplayTag InItemIDTag, int64 InItemCount)
{
	if (!CheckHaveItem(InItemIDTag, InItemCount))
	{
		return false;
	}

	int64 RemainingRemoveCount = InItemCount;
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

		// 此分支剩余扣除量小于单堆数量，差值必定仍在32位物品数量范围内。
		const int32 NewCount = static_cast<int32>(ItemCount - RemainingRemoveCount);
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, FLxItemQuote(InItemIDTag, NewCount));
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

bool ULxCharacterBackpackModule::CheckHaveItem(FGameplayTag InItemIDTag, int64 InItemCount) const
{
	if (!InItemIDTag.IsValid() || InItemCount <= 0)
	{
		return false;
	}

	int64 FoundCount = 0;
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
	if (LxSafeItemOperations::IsContainerBusy(this)) return;
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

ULxItemTransferTicket* ULxCharacterBackpackModule::BeginSafeExtractItem(int32 SlotIndex, FText& OutReason)
{
	return LxSafeItemOperations::BeginSafeExtract(GetBackpackSlotAt(SlotIndex), OutReason);
}

bool ULxCharacterBackpackModule::CanAcceptSafeItem(ULxItemTransferTicket* Ticket, FText& OutReason) const
{
	return LxSafeItemOperations::CanAccept(Ticket, m_vBackpackSlots, OutReason);
}

bool ULxCharacterBackpackModule::AcceptSafeItem(ULxItemTransferTicket* Ticket, FText& OutReason)
{
	return LxSafeItemOperations::Accept(Ticket, m_vBackpackSlots, OutReason);
}

bool ULxCharacterBackpackModule::SplitItemStack(int32 SlotIndex, int32 Count, FText& OutReason)
{
	return LxSafeItemOperations::Split(GetBackpackSlotAt(SlotIndex), m_vBackpackSlots, Count, OutReason);
}

void ULxCharacterBackpackModule::BeginSafeItemBatch()
{
	++SafeItemBatchDepth;
}

void ULxCharacterBackpackModule::EndSafeItemBatch()
{
	if (SafeItemBatchDepth <= 0) return;
	if (--SafeItemBatchDepth > 0) return;
	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	SyncReplicatedBackpackSlots();
}

void ULxCharacterBackpackModule::SyncReplicatedBackpackSlots()
{
	if (SafeItemBatchDepth > 0 || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 完整采集成功后再替换网络快照，避免失败时向客户端发送部分容器。
	TArray<FLxItemSaveRecord> Snapshot;
	Snapshot.Reserve(m_vBackpackSlots.Num());
	for (ULxItemSlotData* SlotData : m_vBackpackSlots)
	{
		FLxItemSaveRecord Record;
        if (!LxItemSaveData::CaptureItem(SlotData ? SlotData->GetItem() : nullptr, Record)) return;
        Snapshot.Add(MoveTemp(Record));
	}
    ReplicatedBackpackSlots = MoveTemp(Snapshot);
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
	if (SafeItemBatchDepth > 0) return;
	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	SyncReplicatedBackpackSlots();
}

void ULxCharacterBackpackModule::HandleBackpackSlotChanged(ULxItemBase* InItemData)
{
	if (SafeItemBatchDepth > 0) return;
	CleanupInvalidItems();
	RefreshTrackedBindings();
	OnDataChange.Broadcast();
	SyncReplicatedBackpackSlots();
}

void ULxCharacterBackpackModule::RefreshTrackedBindings()
{
	// 先解除所有旧实例监听，确保移入仓库的物品不再回调来源背包。
	for (ULxItemBase* Item : TrackedItems)
	{
		if (!Item) continue;
		Item->OnItemCountChanged.RemoveAll(this);
		Item->OnItemUseSucceeded.RemoveAll(this);
	}
	TrackedItems.Reset();
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot == nullptr)
		{
			continue;
		}

		Slot->OnItemDataChanged.RemoveAll(this);
		Slot->OnItemDataChanged.AddUObject(this, &ULxCharacterBackpackModule::HandleBackpackSlotChanged);

		if (Slot->GetItem() != nullptr)
		{
			Slot->GetItem()->OnItemCountChanged.RemoveAll(this);
			Slot->GetItem()->OnItemCountChanged.AddUObject(this, &ULxCharacterBackpackModule::HandleTrackedItemCountChanged);
			Slot->GetItem()->OnItemUseSucceeded.RemoveAll(this);
			Slot->GetItem()->OnItemUseSucceeded.AddUObject(this, &ULxCharacterBackpackModule::NotifyItemUsedFromSlot);
			TrackedItems.AddUnique(Slot->GetItem());
		}
	}
}

void ULxCharacterBackpackModule::CleanupInvalidItems()
{
	for (ULxItemSlotData* Slot : m_vBackpackSlots)
	{
		if (Slot != nullptr && Slot->GetItem() != nullptr && !Slot->GetItem()->ItemIsValid())
		{
			Slot->ClearItem();
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
		NewSlot->OnItemDataChanged.AddUObject(this, &ULxCharacterBackpackModule::HandleBackpackSlotChanged);
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

		ULxItemBase* NewItem = LxItemSaveData::CreateItem(this, ReplicatedBackpackSlots[Index]);
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
	if (!SlotData || !SlotData->IsValid())
	{
		return FLxItemQuote();
	}

	return FLxItemQuote(SlotData->GetItem()->ItemIDTag(), SlotData->GetItem()->ItemCount());
}

void ULxCharacterBackpackModule::OnRep_BackpackSlots()
{
	ApplyReplicatedBackpackSlots();
}

bool ULxCharacterBackpackModule::RestoreBackpackSaveData(int32 InSlotCount,
	const TArray<FLxItemSlotSaveRecord>& InSlots, bool bApply)
{
	if (bApply && LxSafeItemOperations::IsContainerBusy(this)) return false;
	if ((GetOwner() && !GetOwner()->HasAuthority()) || InSlotCount < 0 || InSlotCount > 100000)
	{
		return false;
	}
	TArray<TObjectPtr<ULxItemSlotData>> RestoredSlots;
	RestoredSlots.Reserve(InSlotCount);
	for (int32 Index = 0; Index < InSlotCount; ++Index)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(this);
		Slot->SetSlotIndex(Index);
		Slot->InitItemSlot(ELxItemSlotType::Backpack, LxTag_Item, nullptr);
		RestoredSlots.Add(Slot);
	}
	if (!LxItemSaveData::RestoreSlots(this, InSlots, RestoredSlots))
	{
		return false;
	}
	if (!bApply)
	{
		return true;
	}
	for (ULxItemSlotData* OldSlot : m_vBackpackSlots)
	{
		if (OldSlot)
		{
			OldSlot->OnItemDataChanged.RemoveAll(this);
			if (ULxItemBase* OldItem = OldSlot->GetItem())
			{
				OldItem->OnItemCountChanged.RemoveAll(this);
				OldItem->OnItemUseSucceeded.RemoveAll(this);
			}
		}
	}
	BackpackSlotCount = InSlotCount;
	m_vBackpackSlots = MoveTemp(RestoredSlots);
	CleanupInvalidItems();
	RefreshTrackedBindings();
	SyncReplicatedBackpackSlots();
	OnDataChange.Broadcast();
	return true;
}
