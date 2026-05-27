#include "LxSkillBackpackComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

ULxSkillBackpackComponent::ULxSkillBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxSkillBackpackComponent::BaseComponentInitialize()
{
	RebuildSkillItemSlots();
}

bool ULxSkillBackpackComponent::AddSkillItemByTagID(FGameplayTag InSkillItemIDTag)
{
	if (!InSkillItemIDTag.IsValid())
	{
		return false;
	}

	if (ContainsSkillItem(InSkillItemIDTag))
	{
		return true;
	}

	ULxSkillItem* SkillItem = Cast<ULxSkillItem>(ULxItemBase::CreateItemObject(this, FLxItemQuote(InSkillItemIDTag, 1)));
	return AddSkillItemObject(SkillItem);
}

bool ULxSkillBackpackComponent::AddSkillItemObject(ULxSkillItem* InSkillItem)
{
	if (!InSkillItem || !InSkillItem->ItemIsValid() || InSkillItem->ItemType() != ELxItemType::Skill)
	{
		return false;
	}

	if (ContainsSkillItem(InSkillItem->ItemIDTag()))
	{
		return true;
	}

	SkillItemList.Add(InSkillItem);
	RebuildSkillItemSlots();
	OnDataChange.Broadcast();
	return true;
}

void ULxSkillBackpackComponent::GetAllSkillItemSlots(TArray<ULxItemSlotData*>& OutSkillItemSlots) const
{
	OutSkillItemSlots.Reset();
	for (ULxItemSlotData* SlotData : SkillItemSlotList)
	{
		OutSkillItemSlots.Add(SlotData);
	}
}

void ULxSkillBackpackComponent::QuerySkillItemSlotsByTag(FGameplayTag InSkillTag, TArray<ULxItemSlotData*>& OutSkillItemSlots) const
{
	OutSkillItemSlots.Reset();
	if (!InSkillTag.IsValid())
	{
		GetAllSkillItemSlots(OutSkillItemSlots);
		return;
	}

	for (ULxItemSlotData* SlotData : SkillItemSlotList)
	{
		if (!SlotData || !SlotData->IsValid() || !SlotData->GetItem())
		{
			continue;
		}

		if (SlotData->GetItem()->ItemIDTag().MatchesTag(InSkillTag))
		{
			OutSkillItemSlots.Add(SlotData);
		}
	}
}

void ULxSkillBackpackComponent::GetAllSkillItems(TArray<ULxSkillItem*>& OutSkillItems) const
{
	OutSkillItems.Reset();
	for (ULxSkillItem* SkillItem : SkillItemList)
	{
		OutSkillItems.Add(SkillItem);
	}
}

ULxItemSlotData* ULxSkillBackpackComponent::GetSkillItemSlotAt(int32 SlotIndex) const
{
	return SkillItemSlotList.IsValidIndex(SlotIndex) ? SkillItemSlotList[SlotIndex] : nullptr;
}

void ULxSkillBackpackComponent::RebuildSkillItemSlots()
{
	for (ULxItemSlotData* SlotData : SkillItemSlotList)
	{
		if (SlotData)
		{
			SlotData->OnItemDataChanged.RemoveDynamic(this, &ULxSkillBackpackComponent::HandleSkillSlotChanged);
		}
	}

	SkillItemSlotList.Reset();
	SkillItemSlotList.Reserve(SkillItemList.Num());

	for (int32 Index = 0; Index < SkillItemList.Num(); ++Index)
	{
		ULxSkillItem* SkillItem = SkillItemList[Index];
		if (!SkillItem || !SkillItem->ItemIsValid())
		{
			continue;
		}

		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->InitItemSlot(ELxItemSlotType::SkillDisplay, LxTag_Item_Skill, SkillItem);
		NewSlot->SetSlotIndex(SkillItemSlotList.Num());
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxSkillBackpackComponent::HandleSkillSlotChanged);
		SkillItemSlotList.Add(NewSlot);
	}
}

bool ULxSkillBackpackComponent::ContainsSkillItem(FGameplayTag InSkillItemIDTag) const
{
	if (!InSkillItemIDTag.IsValid())
	{
		return false;
	}

	for (const ULxSkillItem* SkillItem : SkillItemList)
	{
		if (SkillItem && SkillItem->GetSkillItemInformation().ItemIDTag == InSkillItemIDTag)
		{
			return true;
		}
	}

	return false;
}

void ULxSkillBackpackComponent::HandleSkillSlotChanged(ULxItemBase* InItemData)
{
	OnDataChange.Broadcast();
}
