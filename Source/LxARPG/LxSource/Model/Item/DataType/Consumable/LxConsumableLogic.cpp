// Fill out your copyright notice in the Description page of Project Settings.

#include "LxConsumableLogic.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxTextLineStyleData.h"

namespace
{
bool BuildItemEntryData(const FLxItemEntryQuote& InEntryQuote, FLxItemEntryData& OutEntryData)
{
	const FLxItemEntryDefine* EntryDefine = InEntryQuote.ItemEntryDefineTableQuote.GetRow<FLxItemEntryDefine>(TEXT("ULxConsumableLogic"));
	if (EntryDefine == nullptr || EntryDefine->EntryID.IsNone())
	{
		return false;
	}

	OutEntryData.EntryID = EntryDefine->EntryID;
	OutEntryData.DisplayName = EntryDefine->DisplayName;
	OutEntryData.Description = EntryDefine->Description;
	OutEntryData.TextStyle = EntryDefine->TextStyleTableQuote.GetRow<FLxTextLineStyleData>(TEXT("ULxConsumableLogic"));

	const FLxAttributeDefineInfo* AttributeDefine = EntryDefine->AttributeDefineTableQuote.GetRow<FLxAttributeDefineInfo>(TEXT("ULxConsumableLogic"));
	OutEntryData.AttributeID = AttributeDefine ? AttributeDefine->AttributeInfo.AttributeID : NAME_None;

	OutEntryData.ItemEntryDefineValue = EntryDefine->ItemEntryDefineValue;
	OutEntryData.EffectiveRatio = InEntryQuote.UpwardFloatingRatio;
	return true;
}
}

bool ULxConsumableLogic::InitItemLogic(const FLxItemDefineBase* pItemInfo)
{
	if (pItemInfo == nullptr)
	{
		return false;
	}

	if (pItemInfo->ItemInfo.ItemType != ELxItemType::Consumable || pItemInfo->ItemInfo.ItemID.IsNone())
	{
		return false;
	}

	const FLxConsumableDefine* ConsumableDefine = static_cast<const FLxConsumableDefine*>(pItemInfo);

	m_ConsumableData = FLxConsumableData();
	m_ConsumableData.ItemInfo = ConsumableDefine->ItemInfo;
	m_ConsumableData.ItemStackInfo = ConsumableDefine->ItemStackInfo;
	m_ConsumableData.ItemCount = 1;
	m_ConsumableData.ItemShowInfo = ConsumableDefine->ItemShowInfo;
	if (const FLxRarityInfo* RarityInfo = ConsumableDefine->ItemRarityRowQuote.GetRow<FLxRarityInfo>(TEXT("ULxConsumableLogic")))
	{
		m_ConsumableData.ItemRarity = *RarityInfo;
	}
	m_ConsumableData.ConsumableCoreInfo = ConsumableDefine->ConsumableCoreInfo;

	m_ConsumableData.ConsumableEntryInfo.ConsumableEntryList.Empty();
	for (const FLxItemEntryQuote& EntryQuote : ConsumableDefine->ConsumableEntryInfo.ConsumableEntryQuote)
	{
		FLxItemEntryData EntryData;
		if (BuildItemEntryData(EntryQuote, EntryData))
		{
			m_ConsumableData.ConsumableEntryInfo.ConsumableEntryList.Add(EntryData);
		}
	}

	OnItemInfoChanged.Broadcast();
	return true;
}

FLxItemDateBase* ULxConsumableLogic::GetItemDataBase()
{
	return &m_ConsumableData;
}

bool ULxConsumableLogic::UseItem()
{
	m_ConsumableData.ItemCount --;
	return true;
}



