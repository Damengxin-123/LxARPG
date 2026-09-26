#include "LxInteractionSaveHelpers.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

bool LxInteractionSaveHelpers::BuildRestoredSlots(UObject* Outer,
	const TArray<FLxItemSlotSaveRecord>& Records, int32 MinimumSlotCount,
	ELxItemSlotType SlotType, TArray<TObjectPtr<ULxItemSlotData>>& OutSlots)
{
	OutSlots.Reset();
	// 拒绝损坏存档中的异常索引，避免据此分配不受限的槽位数组。
	constexpr int32 MaximumSavedSlotCount = 65536;
	if (!Outer || MinimumSlotCount < 0 || MinimumSlotCount > MaximumSavedSlotCount
		|| Records.Num() > MaximumSavedSlotCount) return false;
	int32 DesiredSlotCount = MinimumSlotCount;
	TSet<int32> SavedIndices;
	for (const FLxItemSlotSaveRecord& Record : Records)
	{
		if (Record.SlotIndex < 0 || Record.SlotIndex >= MaximumSavedSlotCount
			|| SavedIndices.Contains(Record.SlotIndex)) return false;
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
	// 新槽位尚未绑定模块事件，因此恢复过程中不会改写复制快照或触发宝箱领取完成。
	if (!LxItemSaveData::RestoreSlots(Outer, Records, OutSlots))
	{
		OutSlots.Reset();
		return false;
	}
	return true;
}
