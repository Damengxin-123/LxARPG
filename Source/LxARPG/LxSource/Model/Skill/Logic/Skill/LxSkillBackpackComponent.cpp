#include "LxSkillBackpackComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "Net/UnrealNetwork.h"

ULxSkillBackpackModule::ULxSkillBackpackModule()
{
}

void ULxSkillBackpackModule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxSkillBackpackModule, ReplicatedSkillItemIDTags);
}

void ULxSkillBackpackModule::OnModuleInitialize()
{
	RebuildSkillItemSlots();
	SyncReplicatedSkillItemIDTags();
}

bool ULxSkillBackpackModule::AddSkillItemsByTagID(const TArray<FGameplayTag>& InSkillItemIDTags)
{
	bool bAllSucceeded = !InSkillItemIDTags.IsEmpty();
	for (const FGameplayTag SkillItemIDTag : InSkillItemIDTags)
	{
		bAllSucceeded = AddSkillItemByTagID(SkillItemIDTag) && bAllSucceeded;
	}
	return bAllSucceeded;
}

bool ULxSkillBackpackModule::AddSkillItemByTagID(FGameplayTag InSkillItemIDTag)
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

bool ULxSkillBackpackModule::AddSkillItemObject(ULxSkillItem* InSkillItem)
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
	SyncReplicatedSkillItemIDTags();
	return true;
}

void ULxSkillBackpackModule::GetAllSkillItemSlots(TArray<ULxItemSlotData*>& OutSkillItemSlots) const
{
	OutSkillItemSlots.Reset();
	for (ULxItemSlotData* SlotData : SkillItemSlotList)
	{
		OutSkillItemSlots.Add(SlotData);
	}
}

void ULxSkillBackpackModule::QuerySkillItemSlotsByTag(FGameplayTag InSkillTag, TArray<ULxItemSlotData*>& OutSkillItemSlots) const
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

ULxSkillItem* ULxSkillBackpackModule::FindSkillItemByTagID(FGameplayTag InSkillItemIDTag) const
{
	if (!InSkillItemIDTag.IsValid())
	{
		return nullptr;
	}

	for (ULxSkillItem* SkillItem : SkillItemList)
	{
		if (SkillItem && SkillItem->ItemIsValid() && SkillItem->ItemIDTag() == InSkillItemIDTag)
		{
			return SkillItem;
		}
	}
	return nullptr;
}

void ULxSkillBackpackModule::RebuildSkillItemSlots()
{
	for (ULxItemSlotData* SlotData : SkillItemSlotList)
	{
		if (SlotData)
		{
			SlotData->OnItemDataChanged.RemoveDynamic(this, &ULxSkillBackpackModule::HandleSkillSlotChanged);
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
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxSkillBackpackModule::HandleSkillSlotChanged);
		SkillItemSlotList.Add(NewSlot);
	}
}

bool ULxSkillBackpackModule::ContainsSkillItem(FGameplayTag InSkillItemIDTag) const
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

void ULxSkillBackpackModule::HandleSkillSlotChanged(ULxItemBase* InItemData)
{
	OnDataChange.Broadcast();
	SyncReplicatedSkillItemIDTags();
}

void ULxSkillBackpackModule::SyncReplicatedSkillItemIDTags()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedSkillItemIDTags.Reset();
	for (ULxSkillItem* SkillItem : SkillItemList)
	{
		if (SkillItem && SkillItem->ItemIsValid())
		{
			ReplicatedSkillItemIDTags.AddUnique(SkillItem->ItemIDTag());
		}
	}
}

void ULxSkillBackpackModule::OnRep_ReplicatedSkillItemIDTags()
{
	SkillItemList.Reset();
	for (const FGameplayTag SkillItemIDTag : ReplicatedSkillItemIDTags)
	{
		ULxSkillItem* SkillItem = Cast<ULxSkillItem>(
			ULxItemBase::CreateItemObject(this, FLxItemQuote(SkillItemIDTag, 1)));
		if (SkillItem && SkillItem->ItemIsValid())
		{
			SkillItemList.Add(SkillItem);
		}
	}

	RebuildSkillItemSlots();
	OnDataChange.Broadcast();
}
