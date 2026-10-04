#include "LxItemSaveData.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

bool LxItemSaveData::CaptureItem(ULxItemBase* InItem, FLxItemSaveRecord& OutRecord)
{
	FLxItemSaveRecord Record;
	if (!InItem)
	{
		OutRecord = MoveTemp(Record);
		return true;
	}
	if (!IsValid(InItem) || !InItem->ItemIsValid()
		|| InItem->GetItemEntryList().Num() != InItem->GetItemEntryRuntimeInfoList().Num())
	{
		UE_LOG(LogTemp, Error, TEXT("物品存档采集失败：%s 的实例无效或词条运行信息数量不匹配。"), *GetNameSafe(InItem));
		return false;
	}
	Record.ItemIDTag = InItem->ItemIDTag();
	Record.ItemCount = InItem->ItemCount();
	TSet<ULxEntryObjectBase*> CapturedEntries;
	for (ULxEntryObjectBase* Entry : InItem->GetItemEntryList())
	{
		if (!IsValid(Entry) || CapturedEntries.Contains(Entry))
		{
			UE_LOG(LogTemp, Error, TEXT("物品存档采集失败：%s 包含无效或重复的词条对象。"), *GetNameSafe(InItem));
			return false;
		}
		const FLxItemEntryRuntimeInfo* RuntimeInfo = InItem->GetItemEntryRuntimeInfoList().FindByPredicate(
			[Entry](const FLxItemEntryRuntimeInfo& Info) { return Info.EntryObject == Entry; });
		if (!RuntimeInfo)
		{
			UE_LOG(LogTemp, Error, TEXT("物品存档采集失败：%s 的词条 %s 缺少运行信息。"), *GetNameSafe(InItem), *GetNameSafe(Entry));
			return false;
		}
		CapturedEntries.Add(Entry);
		Record.Entries.Emplace(Entry->GetEntryQuote(), RuntimeInfo->EntryLogicType);
	}
	OutRecord = MoveTemp(Record);
	return true;
}

ULxItemBase* LxItemSaveData::CreateItem(UObject* InOuter, const FLxItemSaveRecord& InRecord)
{
	if (!InOuter || !InRecord.ItemIDTag.IsValid() || InRecord.ItemCount <= 0)
	{
		return nullptr;
	}
	ULxItemBase* Item = ULxItemBase::CreateItemObject(InOuter, FLxItemQuote(InRecord.ItemIDTag, InRecord.ItemCount));
	if (!Item || !Item->ItemIsValid() || Item->ItemCount() != InRecord.ItemCount
		|| !Item->RestoreItemEntries(InRecord.Entries))
	{
		return nullptr;
	}
	return Item;
}

bool LxItemSaveData::CaptureSlots(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots,
	TArray<FLxItemSlotSaveRecord>& OutRecords)
{
	TArray<FLxItemSlotSaveRecord> Records;
	Records.Reserve(InSlots.Num());
	for (int32 Index = 0; Index < InSlots.Num(); ++Index)
	{
		ULxItemSlotData* Slot = InSlots[Index];
		if (!IsValid(Slot))
		{
			UE_LOG(LogTemp, Error, TEXT("物品存档采集失败：索引 %d 的槽位对象无效。"), Index);
			return false;
		}
		FLxItemSlotSaveRecord& Record = Records.AddDefaulted_GetRef();
		Record.SlotIndex = Index;
		Record.SlotTag = Slot->GetItemTypeTag();
		if (!CaptureItem(Slot->GetItem(), Record.Item))
		{
			return false;
		}
	}
	OutRecords = MoveTemp(Records);
	return true;
}

bool LxItemSaveData::RestoreSlots(UObject* InOuter, const TArray<FLxItemSlotSaveRecord>& InRecords,
	const TArray<TObjectPtr<ULxItemSlotData>>& InSlots)
{
	TMap<int32, ULxItemBase*> RestoredItems;
	for (const FLxItemSlotSaveRecord& Record : InRecords)
	{
		if (!InSlots.IsValidIndex(Record.SlotIndex) || !InSlots[Record.SlotIndex]
			|| RestoredItems.Contains(Record.SlotIndex))
		{
			return false;
		}
		ULxItemSlotData* Slot = InSlots[Record.SlotIndex];
		if (Record.SlotTag.IsValid() && Record.SlotTag != Slot->GetItemTypeTag())
		{
			return false;
		}
		ULxItemBase* Item = nullptr;
		if (Record.Item.ItemIDTag.IsValid())
		{
			Item = CreateItem(InOuter, Record.Item);
			if (!Item || (Slot->GetItemTypeTag().IsValid() && !Item->ItemIDTag().MatchesTag(Slot->GetItemTypeTag())))
			{
				return false;
			}
		}
		else if (Record.Item.ItemCount != 0 || !Record.Item.Entries.IsEmpty())
		{
			return false;
		}
		RestoredItems.Add(Record.SlotIndex, Item);
	}
	for (int32 Index = 0; Index < InSlots.Num(); ++Index)
	{
		if (ULxItemSlotData* Slot = InSlots[Index])
		{
			if (ULxItemBase* Item = RestoredItems.FindRef(Index))
			{
				Slot->SetItem(Item);
			}
			else
			{
				Slot->ClearItem();
			}
		}
	}
	return true;
}
