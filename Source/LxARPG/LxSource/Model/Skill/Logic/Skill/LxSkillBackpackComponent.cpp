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
		// 永久学习已由装备授予的技能时，也需要解除卸装回收标记。
		EquipmentOnlySkillItemIDTags.Remove(SkillItemIDTag);
		bAllSucceeded = AddSkillItemByTagID(SkillItemIDTag) && bAllSucceeded;
	}
	return bAllSucceeded;
}

void ULxSkillBackpackModule::SyncEquipmentGrantedSkillItems(const TArray<FGameplayTag>& InSkillItemIDTags)
{
	TSet<FGameplayTag> NewEquipmentSkillTags;
	for (const FGameplayTag SkillItemIDTag : InSkillItemIDTags)
	{
		if (SkillItemIDTag.IsValid())
		{
			NewEquipmentSkillTags.Add(SkillItemIDTag);
		}
	}

	TArray<TObjectPtr<ULxSkillItem>> RemovedSkillItems;
	for (int32 Index = SkillItemList.Num() - 1; Index >= 0; --Index)
	{
		ULxSkillItem* SkillItem = SkillItemList[Index];
		if (SkillItem && EquipmentOnlySkillItemIDTags.Contains(SkillItem->ItemIDTag())
			&& !NewEquipmentSkillTags.Contains(SkillItem->ItemIDTag()))
		{
			EquipmentOnlySkillItemIDTags.Remove(SkillItem->ItemIDTag());
			RemovedSkillItems.Add(SkillItem);
			SkillItemList.RemoveAt(Index);
		}
	}

	bool bItemsChanged = !RemovedSkillItems.IsEmpty();
	for (const FGameplayTag SkillItemIDTag : InSkillItemIDTags)
	{
		if (!SkillItemIDTag.IsValid() || ContainsSkillItem(SkillItemIDTag))
		{
			continue;
		}

		ULxSkillItem* SkillItem = Cast<ULxSkillItem>(
			ULxItemBase::CreateItemObject(this, FLxItemQuote(SkillItemIDTag, 1)));
		if (SkillItem && SkillItem->ItemIsValid())
		{
			SkillItemList.Add(SkillItem);
			EquipmentOnlySkillItemIDTags.Add(SkillItemIDTag);
			bItemsChanged = true;
		}
	}

	if (bItemsChanged)
	{
		// 先更新列表并解绑旧展示槽，再通知快捷栏中的共享技能引用失效。
		RebuildSkillItemSlots();
		for (ULxSkillItem* SkillItem : RemovedSkillItems)
		{
			SkillItem->InvalidateSkillItem();
		}
		SyncReplicatedSkillItemIDTags();
		OnDataChange.Broadcast();
	}
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
			SlotData->OnItemDataChanged.RemoveAll(this);
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
		NewSlot->OnItemDataChanged.AddUObject(this, &ULxSkillBackpackModule::HandleSkillSlotChanged);
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
	TArray<TObjectPtr<ULxSkillItem>> RemovedSkillItems;
	for (int32 Index = SkillItemList.Num() - 1; Index >= 0; --Index)
	{
		ULxSkillItem* SkillItem = SkillItemList[Index];
		if (!SkillItem || !ReplicatedSkillItemIDTags.Contains(SkillItem->ItemIDTag()))
		{
			if (SkillItem)
			{
				RemovedSkillItems.Add(SkillItem);
			}
			SkillItemList.RemoveAt(Index);
		}
	}
	for (const FGameplayTag SkillItemIDTag : ReplicatedSkillItemIDTags)
	{
		// 保留仍拥有的技能实例，使快捷栏引用与技能背包保持一致。
		if (ContainsSkillItem(SkillItemIDTag))
		{
			continue;
		}
		ULxSkillItem* SkillItem = Cast<ULxSkillItem>(
			ULxItemBase::CreateItemObject(this, FLxItemQuote(SkillItemIDTag, 1)));
		if (SkillItem && SkillItem->ItemIsValid())
		{
			SkillItemList.Add(SkillItem);
		}
	}

	RebuildSkillItemSlots();
	for (ULxSkillItem* SkillItem : RemovedSkillItems)
	{
		SkillItem->InvalidateSkillItem();
	}
	OnDataChange.Broadcast();
}
