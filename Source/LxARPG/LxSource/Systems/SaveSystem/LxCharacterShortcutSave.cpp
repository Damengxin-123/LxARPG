#include "LxCharacterSaveComponent.h"

#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxItemSlotData* ULxCharacterSaveComponent::GetShortcutSlot(int32 ShortcutIndex)
{
	if (ShortcutIndex < 0 || ShortcutIndex >= ShortcutSlotCount) return nullptr;
	ShortcutSlots.SetNum(ShortcutSlotCount);
	if (!ShortcutSlots[ShortcutIndex])
	{
		ShortcutSlots[ShortcutIndex] = NewObject<ULxItemSlotData>(this);
		ShortcutSlots[ShortcutIndex]->InitItemSlot(ELxItemSlotType::Shortcut);
		ShortcutSlots[ShortcutIndex]->SetSlotIndex(ShortcutIndex);
	}
	return ShortcutSlots[ShortcutIndex];
}

void ULxCharacterSaveComponent::CaptureShortcutSaveData(TArray<FLxShortcutSaveRecord>& OutRecords) const
{
	OutRecords.SetNum(ShortcutSlotCount);
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	const ULxCharacterContentComponent* Content = Character ? Character->GetCharacterContentComponent() : nullptr;
	if (!Content) return;
	for (int32 Index = 0; Index < ShortcutSlotCount; ++Index)
	{
		FLxShortcutSaveRecord& Record = OutRecords[Index];
		Record = FLxShortcutSaveRecord();
		ULxItemBase* Item = ShortcutSlots.IsValidIndex(Index) && ShortcutSlots[Index] ? ShortcutSlots[Index]->GetItem() : nullptr;
		if (!Item || !Item->ItemIsValid()) continue;
		// 物品移动或排序后，以同一个实例当前所在的槽位为准，区分同标签的不同物品堆。
		const auto FindSource = [Item, &Record](const TArray<TObjectPtr<ULxItemSlotData>>& Slots, ELxShortcutItemSource Source)
		{
			for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
			{
				if (Slots[SlotIndex] && Slots[SlotIndex]->GetItem() == Item)
				{
					Record.Source = Source;
					Record.SlotIndex = SlotIndex;
					Record.ItemIDTag = Item->ItemIDTag();
					return true;
				}
			}
			return false;
		};
		if (Content->GetBackpackModule() && FindSource(Content->GetBackpackModule()->GetAllItems(), ELxShortcutItemSource::Backpack)) continue;
		if (Content->GetEquipmentModule() && FindSource(Content->GetEquipmentModule()->GetEquipmentSlots(), ELxShortcutItemSource::Equipment)) continue;
		if (Content->GetSkillBackpackModule() && Content->GetSkillBackpackModule()->FindSkillItemByTagID(Item->ItemIDTag()) == Item)
		{
			Record.Source = ELxShortcutItemSource::SkillBackpack;
			Record.ItemIDTag = Item->ItemIDTag();
		}
	}
}

bool ULxCharacterSaveComponent::ValidateShortcutSaveData(const TArray<FLxShortcutSaveRecord>& Records)
{
	if (Records.Num() > ShortcutSlotCount) return false;
	for (const FLxShortcutSaveRecord& Record : Records)
	{
		switch (Record.Source)
		{
		case ELxShortcutItemSource::None:
			if (Record.SlotIndex != INDEX_NONE || Record.ItemIDTag.IsValid()) return false;
			break;
		case ELxShortcutItemSource::Backpack:
		case ELxShortcutItemSource::Equipment:
			if (Record.SlotIndex < 0 || !Record.ItemIDTag.IsValid()) return false;
			break;
		case ELxShortcutItemSource::SkillBackpack:
			if (Record.SlotIndex != INDEX_NONE || !Record.ItemIDTag.IsValid()) return false;
			break;
		default:
			return false;
		}
	}
	return true;
}

void ULxCharacterSaveComponent::RestoreShortcutSaveData(const TArray<FLxShortcutSaveRecord>& Records)
{
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	const ULxCharacterContentComponent* Content = Character ? Character->GetCharacterContentComponent() : nullptr;
	for (int32 Index = 0; Index < ShortcutSlotCount; ++Index)
	{
		ULxItemSlotData* Shortcut = GetShortcutSlot(Index);
		ULxItemBase* Item = nullptr;
		if (Content && Records.IsValidIndex(Index))
		{
			const FLxShortcutSaveRecord& Record = Records[Index];
			if (Record.Source == ELxShortcutItemSource::Backpack && Content->GetBackpackModule())
			{
				if (ULxItemSlotData* Slot = Content->GetBackpackModule()->GetBackpackSlotAt(Record.SlotIndex)) Item = Slot->GetItem();
			}
			else if (Record.Source == ELxShortcutItemSource::Equipment && Content->GetEquipmentModule())
			{
				const auto& Slots = Content->GetEquipmentModule()->GetEquipmentSlots();
				if (Slots.IsValidIndex(Record.SlotIndex) && Slots[Record.SlotIndex]) Item = Slots[Record.SlotIndex]->GetItem();
			}
			else if (Record.Source == ELxShortcutItemSource::SkillBackpack && Content->GetSkillBackpackModule())
			{
				Item = Content->GetSkillBackpackModule()->FindSkillItemByTagID(Record.ItemIDTag);
			}
			// 已消耗、失去技能或缺失来源时清空快捷格，不重新生成物品或授予技能。
			if (Item && Item->ItemIDTag() != Record.ItemIDTag) Item = nullptr;
		}
		Shortcut->ClearItem(false);
		if (Item) Shortcut->SetItem(Item, false);
		Shortcut->NotifyItemDataChanged();
	}
}
