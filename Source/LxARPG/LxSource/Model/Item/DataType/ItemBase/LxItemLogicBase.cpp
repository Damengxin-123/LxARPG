// Fill out your copyright notice in the Description page of Project Settings.


#include "LxItemLogicBase.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"

bool ULxItemLogicBase::BuildItemEntryData(const FLxItemEntryQuote& InEntryQuote, FLxItemEntryData& OutEntryData)
{
	// 先从引用里取词条定义
	const FLxItemEntryDefine* EntryDefine = InEntryQuote.ItemEntryDefineTableQuote.GetRow<FLxItemEntryDefine>(TEXT("ULxEquipmentLogic"));
	if (EntryDefine == nullptr || EntryDefine->EntryID.IsNone())
	{
		return false;
	}

	OutEntryData.EntryID = EntryDefine->EntryID;
	OutEntryData.DisplayNameData = EntryDefine->DisplayNameData;
	OutEntryData.Description = EntryDefine->Description;

	// 解析词条作用到的属性定义（可能为空
	OutEntryData.AttributeID = EntryDefine->AttributeTypeID ;
	
	OutEntryData.ItemEntryDefineValue = EntryDefine->ItemEntryDefineValue;
	OutEntryData.ItemEntryLogicType = EntryDefine->ItemEntryLogicType;
	OutEntryData.EffectiveRatio = InEntryQuote.UpwardFloatingRatio;

	return true;
}

bool ULxItemLogicBase::ItemIsStack(ULxItemLogicBase* InItemLogic)
{
	if (InItemLogic == nullptr || InItemLogic == this)
	{
		return false;
	}

	if (!ItemIsValid() || !InItemLogic->ItemIsValid())
	{
		return false;
	}

	FLxItemDateBase* ThisItemData = GetItemDataBase();
	FLxItemDateBase* SourceItemData = InItemLogic->GetItemDataBase();
	if (ThisItemData == nullptr || SourceItemData == nullptr)
	{
		return false;
	}

	if (!ThisItemData->ItemStackInfo.ItemCanStack)
	{
		return false;
	}

	if (ThisItemData->ItemInfo.ItemType == ELxItemType::Equipment
		|| ThisItemData->ItemInfo.ItemType == ELxItemType::Skill
		|| ThisItemData->ItemInfo.ItemType == ELxItemType::Buff)
	{
		return false;
	}

	if (SourceItemData->ItemInfo.ItemType != ThisItemData->ItemInfo.ItemType
		|| SourceItemData->ItemInfo.ItemID != ThisItemData->ItemInfo.ItemID)
	{
		return false;
	}

	if (ThisItemData->ItemCount >= ThisItemData->ItemStackInfo.ItemMaxCount)
	{
		return false;
	}

	return true;
}

bool ULxItemLogicBase::StackItem(ULxItemLogicBase* SourceItemLogic)
{
	if (!ItemIsStack(SourceItemLogic))
	{
		return false;
	}

	FLxItemDateBase* ThisItemData = GetItemDataBase();
	FLxItemDateBase* SourceItemData = SourceItemLogic->GetItemDataBase();
	if (ThisItemData == nullptr || SourceItemData == nullptr)
	{
		return false;
	}

	const int32 Remaining = ThisItemData->ItemStackInfo.ItemMaxCount - ThisItemData->ItemCount;
	const int32 MoveCount = FMath::Min(Remaining, SourceItemData->ItemCount);
	if (MoveCount <= 0)
	{
		return false;
	}

	ThisItemData->ItemCount += MoveCount;
	SourceItemData->ItemCount -= MoveCount;

	OnItemInfoChanged.Broadcast();
	SourceItemLogic->OnItemInfoChanged.Broadcast();
	return true;
}

bool ULxItemLogicBase::ItemIsValid()
{
	FLxItemDateBase* ItemData = GetItemDataBase();
	if (ItemData == nullptr)
	{
		return false;
	}

	return ItemData->ItemInfo.ItemType != ELxItemType::None
		&& !ItemData->ItemInfo.ItemID.IsNone()
		&& ItemData->ItemCount > 0
		&& ItemData->ItemCount != ERR_ATTRIBUTE;
}

bool ULxItemLogicBase::operator<(const ULxItemLogicBase* Other) const
{
	
		if (Other == nullptr || Other == this)
		{
			return false;
		}

		FLxItemDateBase* ThisItemData = const_cast<ULxItemLogicBase*>(this)->GetItemDataBase();
		FLxItemDateBase* OtherItemData = const_cast<ULxItemLogicBase*>(Other)->GetItemDataBase();
		if (ThisItemData == nullptr || OtherItemData == nullptr)
		{
			return false;
		}

		if (ThisItemData->ItemInfo.ItemType != OtherItemData->ItemInfo.ItemType)
		{
			return static_cast<int32>(ThisItemData->ItemInfo.ItemType) < static_cast<int32>(OtherItemData->ItemInfo.ItemType);
		}

		return ThisItemData->ItemRarity.RarityValue < OtherItemData->ItemRarity.RarityValue;

}

bool ULxItemLogicBase::operator>(const ULxItemLogicBase* Other) const
{
	if (Other == nullptr || Other == this)
	{
		return false;
	}

	FLxItemDateBase* ThisItemData = const_cast<ULxItemLogicBase*>(this)->GetItemDataBase();
	FLxItemDateBase* OtherItemData = const_cast<ULxItemLogicBase*>(Other)->GetItemDataBase();
	if (ThisItemData == nullptr || OtherItemData == nullptr)
	{
		return false;
	}

	if (ThisItemData->ItemInfo.ItemType != OtherItemData->ItemInfo.ItemType)
	{
		return static_cast<int32>(ThisItemData->ItemInfo.ItemType) > static_cast<int32>(OtherItemData->ItemInfo.ItemType);
	}

	return ThisItemData->ItemRarity.RarityValue > OtherItemData->ItemRarity.RarityValue;
}
