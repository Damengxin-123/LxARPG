#include "LxItemSaveData.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

FLxItemSaveRecord LxItemSaveData::CaptureItem(ULxItemBase* InItem)
{
	FLxItemSaveRecord Record;
	if (!InItem || !InItem->ItemIsValid())
	{
		return Record;
	}
	Record.ItemIDTag = InItem->ItemIDTag();
	Record.ItemCount = InItem->ItemCount();
	for (ULxEntryObjectBase* Entry : InItem->GetItemEntryList())
	{
		if (!Entry)
		{
			continue;
		}
		const FLxItemEntryRuntimeInfo* RuntimeInfo = InItem->GetItemEntryRuntimeInfoList().FindByPredicate(
			[Entry](const FLxItemEntryRuntimeInfo& Info) { return Info.EntryObject == Entry; });
		Record.Entries.Emplace(Entry->GetEntryQuote(), RuntimeInfo ? RuntimeInfo->EntryLogicType : ELxEntryLogicType::Normal);
	}
	return Record;
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

void LxItemSaveData::CaptureSlots(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots,
	TArray<FLxItemSlotSaveRecord>& OutRecords)
{
	OutRecords.Reset(InSlots.Num());
	for (int32 Index = 0; Index < InSlots.Num(); ++Index)
	{
		FLxItemSlotSaveRecord& Record = OutRecords.AddDefaulted_GetRef();
		Record.SlotIndex = Index;
		if (ULxItemSlotData* Slot = InSlots[Index])
		{
			Record.SlotTag = Slot->GetItemTypeTag();
			Record.Item = CaptureItem(Slot->GetItem());
		}
	}
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
