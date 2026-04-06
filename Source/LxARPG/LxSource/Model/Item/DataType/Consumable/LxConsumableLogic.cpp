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

const FLxItemDateBase* ULxConsumableLogic::GetItemDataBase() const
{
	return &m_ConsumableData;
}

bool ULxConsumableLogic::UseItem()
{
	return false;
}

bool ULxConsumableLogic::ItemIsStack()
{
	return m_ConsumableData.ItemStackInfo.ItemCanStack;
}

bool ULxConsumableLogic::StackItem(ULxItemLogicBase* SourceItemLogic)
{
	if (SourceItemLogic == nullptr || SourceItemLogic == this)
	{
		return false;
	}

	if (!ItemIsStack())
	{
		return false;
	}

	ULxConsumableLogic* SourceConsumableLogic = Cast<ULxConsumableLogic>(SourceItemLogic);
	if (SourceConsumableLogic == nullptr)
	{
		return false;
	}

	if (m_ConsumableData.ItemInfo.ItemID != SourceConsumableLogic->m_ConsumableData.ItemInfo.ItemID)
	{
		return false;
	}

	const int32 MaxCount = m_ConsumableData.ItemStackInfo.ItemMaxCount;
	const int32 Remaining = MaxCount - m_ConsumableData.ItemCount;
	if (Remaining <= 0)
	{
		return false;
	}

	const int32 MoveCount = FMath::Min(Remaining, SourceConsumableLogic->m_ConsumableData.ItemCount);
	if (MoveCount <= 0)
	{
		return false;
	}

	m_ConsumableData.ItemCount += MoveCount;
	SourceConsumableLogic->m_ConsumableData.ItemCount -= MoveCount;

	OnItemInfoChanged.Broadcast();
	SourceConsumableLogic->OnItemInfoChanged.Broadcast();
	return true;
}

bool ULxConsumableLogic::ItemIsValid()
{
	return m_ConsumableData.ItemInfo.ItemType == ELxItemType::Consumable
		&& !m_ConsumableData.ItemInfo.ItemID.IsNone()
		&& m_ConsumableData.ItemCount > 0
		&& m_ConsumableData.ItemCount != ERR_ATTRIBUTE;
}

bool ULxConsumableLogic::operator<(const ULxItemLogicBase* Other) const
{
	const ULxConsumableLogic* OtherConsumable = Cast<ULxConsumableLogic>(Other);
	if (OtherConsumable == nullptr)
	{
		return false;
	}

	if (m_ConsumableData.ItemRarity.RarityValue != OtherConsumable->m_ConsumableData.ItemRarity.RarityValue)
	{
		return m_ConsumableData.ItemRarity.RarityValue < OtherConsumable->m_ConsumableData.ItemRarity.RarityValue;
	}

	return m_ConsumableData.ItemInfo.ItemID.LexicalLess(OtherConsumable->m_ConsumableData.ItemInfo.ItemID);
}

bool ULxConsumableLogic::operator>(const ULxItemLogicBase* Other) const
{
	const ULxConsumableLogic* OtherConsumable = Cast<ULxConsumableLogic>(Other);
	if (OtherConsumable == nullptr)
	{
		return false;
	}

	if (m_ConsumableData.ItemRarity.RarityValue != OtherConsumable->m_ConsumableData.ItemRarity.RarityValue)
	{
		return m_ConsumableData.ItemRarity.RarityValue > OtherConsumable->m_ConsumableData.ItemRarity.RarityValue;
	}

	return OtherConsumable->m_ConsumableData.ItemInfo.ItemID.LexicalLess(m_ConsumableData.ItemInfo.ItemID);
}
