#include "LxInteractionSaveHelpers.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

bool LxInteractionSaveHelpers::CaptureSlots(const TArray<TObjectPtr<ULxItemSlotData>>& Slots,
	TArray<FLxInteractionItemSaveRecord>& OutRecords)
{
	TArray<FLxInteractionItemSaveRecord> Records;
	Records.Reserve(Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!IsValid(Slots[Index])) return false;
		FLxInteractionItemSaveRecord& Record = Records.AddDefaulted_GetRef();
		Record.SlotIndex = Index;
		if (ULxItemBase* Item = Slots[Index]->GetItem())
		{
			if (!IsValid(Item) || !Item->ItemIsValid()) return false;
			Record.ItemIDTag = Item->ItemIDTag();
			Record.ItemCount = Item->ItemCount();
		}
	}
	OutRecords = MoveTemp(Records);
	return true;
}

bool LxInteractionSaveHelpers::ReadItemSlots(const FLxInteractionFeatureSaveRecord& Record,
	TArray<FLxInteractionItemSaveRecord>& OutRecords)
{
	if (Record.DataVersion == 1)
	{
		OutRecords = Record.ItemSlots;
		return true;
	}
	if (Record.DataVersion != 0) return false;
	TArray<FLxInteractionItemSaveRecord> Records;
	Records.Reserve(Record.Slots.Num());
	for (const FLxItemSlotSaveRecord& Legacy : Record.Slots)
	{
		if (Legacy.SlotTag.IsValid() && Legacy.SlotTag != LxTag_Item) return false;
		FLxInteractionItemSaveRecord& Item = Records.AddDefaulted_GetRef();
		Item.SlotIndex = Legacy.SlotIndex;
		Item.ItemIDTag = Legacy.Item.ItemIDTag;
		Item.ItemCount = Legacy.Item.ItemCount;
	}
	OutRecords = MoveTemp(Records);
	return true;
}

bool LxInteractionSaveHelpers::BuildRestoredSlots(UObject* Outer,
	const TArray<FLxInteractionItemSaveRecord>& Records, int32 MinimumSlotCount,
	ELxItemSlotType SlotType, TArray<TObjectPtr<ULxItemSlotData>>& OutSlots)
{
	OutSlots.Reset();
	// 拒绝损坏存档中的异常索引，避免据此分配不受限的槽位数组。
	constexpr int32 MaximumSavedSlotCount = 65536;
	if (!Outer || MinimumSlotCount < 0 || MinimumSlotCount > MaximumSavedSlotCount
		|| Records.Num() > MaximumSavedSlotCount) return false;
	int32 DesiredSlotCount = MinimumSlotCount;
	TSet<int32> SavedIndices;
	for (const FLxInteractionItemSaveRecord& Record : Records)
	{
		if (Record.SlotIndex < 0 || Record.SlotIndex >= MaximumSavedSlotCount
			|| SavedIndices.Contains(Record.SlotIndex) || Record.ItemCount < 0
			|| Record.ItemCount > TNumericLimits<FLxItemCount>::Max()
			|| (Record.ItemIDTag.IsValid() != (Record.ItemCount > 0))) return false;
		SavedIndices.Add(Record.SlotIndex);
		DesiredSlotCount = FMath::Max(DesiredSlotCount, Record.SlotIndex + 1);
	}
	OutSlots.Reserve(DesiredSlotCount);
	for (int32 Index = 0; Index < DesiredSlotCount; ++Index)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(Outer);
		Slot->SetSlotIndex(Index);
		Slot->InitItemSlot(SlotType, LxTag_Item, nullptr);
		OutSlots.Add(Slot);
	}
	// 直接从预设重建，避免通用物品恢复接口用空实例词条覆盖预设词条。
	// 新槽位尚未绑定模块事件，恢复过程中不会触发宝箱领取完成。
	for (const FLxInteractionItemSaveRecord& Record : Records)
	{
		if (!Record.ItemIDTag.IsValid() && Record.ItemCount == 0) continue;
		ULxItemBase* Item = Record.ItemIDTag.IsValid() && Record.ItemCount > 0
			? ULxItemBase::CreateItemObject(Outer, FLxItemQuote(Record.ItemIDTag, Record.ItemCount)) : nullptr;
		if (!Item || !Item->ItemIsValid() || Item->ItemCount() != Record.ItemCount
			|| !OutSlots[Record.SlotIndex]->SetItem(Item))
		{
			OutSlots.Reset();
			return false;
		}
	}
	return true;
}
