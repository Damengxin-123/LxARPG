#include "LxCharacterSaveData.h"

bool FLxCharacterSaveRecord::ExtractEquipmentInstances()
{
    FLxCharacterSaveRecord Copy = *this;
    Copy.EquipmentInstances.Reset();
    for (TArray<FLxItemSlotSaveRecord>* Slots : { &Copy.BackpackSlots, &Copy.EquipmentSlots })
    {
        for (FLxItemSlotSaveRecord& Slot : *Slots)
        {
            FLxItemSaveRecord& Item = Slot.Item;
            if (!Item.bHasEquipmentState) continue;
            if (!Item.EquipmentInstanceID.IsValid() || Copy.EquipmentInstances.Contains(Item.EquipmentInstanceID)) return false;
            Copy.EquipmentInstances.Add(Item.EquipmentInstanceID, Item.Equipment);
            const FGuid ID = Item.EquipmentInstanceID;
            Item = FLxItemSaveRecord();
            Item.EquipmentInstanceID = ID;
        }
    }
    *this = MoveTemp(Copy);
    return true;
}

bool FLxCharacterSaveRecord::ResolveEquipmentInstances(TArray<FLxItemSlotSaveRecord>& OutBackpack,
    TArray<FLxItemSlotSaveRecord>& OutEquipment) const
{
    TArray<FLxItemSlotSaveRecord> Backpack = BackpackSlots;
    TArray<FLxItemSlotSaveRecord> Equipment = EquipmentSlots;
    TSet<FGuid> Seen;
    for (TArray<FLxItemSlotSaveRecord>* Slots : { &Backpack, &Equipment })
    {
        for (FLxItemSlotSaveRecord& Slot : *Slots)
        {
            FLxItemSaveRecord& Item = Slot.Item;
            if (!Item.EquipmentInstanceID.IsValid()) continue; // 旧档保持内联格式读取。
            if (Seen.Contains(Item.EquipmentInstanceID)) return false;
            Seen.Add(Item.EquipmentInstanceID);
            if (Item.bHasEquipmentState && EquipmentInstances.IsEmpty()) continue;
            const FLxEquipmentSaveRecord* State = EquipmentInstances.Find(Item.EquipmentInstanceID);
            if (!State || Item.ItemIDTag.IsValid() || Item.ItemCount != 0 || Item.bHasEquipmentState || !Item.Entries.IsEmpty()) return false;
            Item.bHasEquipmentState = true;
            Item.Equipment = *State;
            Item.ItemIDTag = State->EquipmentID;
            Item.ItemCount = 1;
        }
    }
    if (!EquipmentInstances.IsEmpty() && Seen.Num() != EquipmentInstances.Num()) return false;
    OutBackpack = MoveTemp(Backpack);
    OutEquipment = MoveTemp(Equipment);
    return true;
}
