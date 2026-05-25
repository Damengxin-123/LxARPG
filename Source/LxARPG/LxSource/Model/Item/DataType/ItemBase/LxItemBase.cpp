#include "LxItemBase.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterial.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"

ULxItemBase* ULxItemBase::CreateItemObject(UObject* InParent, FLxItemQuote InItemQuote)
{
	ULxItemBase* OutItemObject = nullptr;
	const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(InItemQuote.ItemIDTag);
	if (ItemConfig == nullptr)
	{
		return nullptr;
	}

	switch (ItemConfig->ItemType)
	{
	case ELxItemType::Equipment:
		OutItemObject = NewObject<ULxEquipment>(InParent);
		break;
	case ELxItemType::Consumable:
		OutItemObject = NewObject<ULxConsumable>(InParent);
		break;
	case ELxItemType::Material:
		OutItemObject = NewObject<ULxMaterial>(InParent);
		break;
	case ELxItemType::Buff:
		OutItemObject = NewObject<ULxBuff>(InParent);
		break;
	case ELxItemType::Skill:
		OutItemObject = NewObject<ULxSkillItem>(InParent);
		break;
	default:
		return nullptr;
	}

	if (OutItemObject != nullptr)
	{
		OutItemObject->InitItemObject(InItemQuote);
	}

	return OutItemObject;
}

void ULxItemBase::InitItemObject(const FLxItemQuote& InItemQuote)
{
	SetItemData(LxItemConfig::GetItemData(InItemQuote.ItemIDTag), InItemQuote.ItemCount);
	InitItemEntry();
}

bool ULxItemBase::ItemIsStackable()
{
	if (ItemBase())
	{
		if (ItemBase() && ItemBase()->ItemCountMax > 1 && ItemBase()->ItemCount < ItemBase()->ItemCountMax)
		{
			return true;
		}
	}
	return false;
}

FGameplayTag ULxItemBase::ItemIDTag()
{
	if (ItemBase())
	{
		return ItemBase()->ItemIDTag;
	}
	return FGameplayTag();
}

ELxItemType ULxItemBase::ItemType()
{
	if (ItemBase())
	{
		return ItemBase()->ItemType;
	}
	return ELxItemType::None;
}

ELxItemRarityType ULxItemBase::ItemRarity()
{
	if (ItemBase())
	{
		return ItemBase()->ItemRarity;
	}
	return ELxItemRarityType::None;
}

FText ULxItemBase::ItemDisplayName()
{
	if (ItemBase())
	{
		return ItemBase()->ItemDisplayName;
	}
	return FText::GetEmpty();
}

FText ULxItemBase::ItemDisplayDescription()
{
	if (ItemBase())
	{
		return ItemBase()->ItemDisplayDescription;
	}
	return FText::GetEmpty();
}

TSoftObjectPtr<UTexture2D> ULxItemBase::ItemIcon()
{
	if (ItemBase())
	{
		return ItemBase()->ItemIcon;
	}
	return TSoftObjectPtr<UTexture2D>();
}

FLxItemInformationBase ULxItemBase::ItemInformation()
{
	if (ItemBase())
	{
		return *ItemBase();
	}
	return FLxItemInformationBase();
}

FLxItemCount ULxItemBase::ItemCount()
{
	if (ItemBase())
	{
		return ItemBase()->ItemCount;
	}
	return 0;
}

bool ULxItemBase::ItemStack(ULxItemBase* InItem)
{
	if (InItem && InItem->ItemIsValid())
	{
		FLxItemInformationBase* ThisItemInfo = ItemBase();
		FLxItemInformationBase* InItemInfo = InItem->ItemBase();
		if (ThisItemInfo && InItemInfo)
		{
			if (ThisItemInfo->ItemCountMax > 1
				&& ThisItemInfo->ItemCount < ThisItemInfo->ItemCountMax
				&& ThisItemInfo->ItemIDTag == InItemInfo->ItemIDTag)
			{
				FLxItemCount ItemCount = ThisItemInfo->ItemCountMax - ThisItemInfo->ItemCount;
				if (InItemInfo->ItemCount > ItemCount)
				{
					ThisItemInfo->ItemCount = ThisItemInfo->ItemCountMax;
					InItemInfo->ItemCount -= ItemCount;
					return true;
				}
				if (InItemInfo->ItemCount <= ItemCount)
				{
					ThisItemInfo->ItemCount += InItemInfo->ItemCount;
					InItemInfo->ItemCount = 0;
					return true;
				}
			}
		}
	}
	return true;
}

bool ULxItemBase::ItemIsValid()
{
	if (ItemBase())
	{
		if (ItemBase()->ItemCount > 0)
		{
			return true;
		}
	}
	return false;
}

bool ULxItemBase::operator<(ULxItemBase& InItem)
{
	if (ItemType() != InItem.ItemType())
	{
		return ItemType() < InItem.ItemType();
	}
	return ItemRarity() < InItem.ItemRarity();
}

bool ULxItemBase::operator>(ULxItemBase& InItem)
{
	if (ItemType() != InItem.ItemType())
	{
		return ItemType() > InItem.ItemType();
	}
	return ItemRarity() > InItem.ItemRarity();
}

bool ULxItemBase::operator==(ULxItemBase& InItem)
{
	return ItemIDTag() == InItem.ItemIDTag();
}

void ULxItemBase::InitItemEntry()
{
	if (FLxItemInformationBase* Item = ItemBase())
	{
		for (FLxEntryQuote& EntryQuote : Item->ItemEntryQuotes)
		{
			if (ULxEntryObjectBase* EntryBase = ULxEntryObjectBase::CreateEnterObject(this, EntryQuote))
			{
				ItemEntryArray.Add(EntryBase);
			}
		}
	}
}

TArray<TObjectPtr<ULxEntryObjectBase>>& ULxItemBase::GetItemEntryList()
{
	return ItemEntryArray;
}

void ULxItemBase::BroadcastItemCountChanged()
{
	OnItemCountChanged.Broadcast(this);
}

ELxItemUseState ULxItemBase::ItemUseStart()
{
	return ELxItemUseState::Failed;
}

ELxItemUseState ULxItemBase::ItemUseEnd()
{
	return ItemUse();
}

void ULxItemBase::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
}
