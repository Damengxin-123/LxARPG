// Fill out your copyright notice in the Description page of Project Settings.

#include "LxBuffLogic.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxItemEntryLogic.h"

ULxBuffLogic* ULxBuffLogic::CreateBuffLogicObject(const FLxBuffDefine& InBuffDefine, UObject* InOuter)
{
	if (InOuter == nullptr)
	{
		return nullptr;
	}

	ULxBuffLogic* NewBuffLogic = NewObject<ULxBuffLogic>(InOuter);
	if (NewBuffLogic == nullptr)
	{
		return nullptr;
	}

	if (!NewBuffLogic->InitItemLogic(&InBuffDefine))
	{
		return nullptr;
	}

	return NewBuffLogic;
}

bool ULxBuffLogic::InitItemLogic(const FLxItemDefineBase* pItemInfo)
{
	if (pItemInfo == nullptr)
	{
		return false;
	}

	if (pItemInfo->ItemInfo.ItemType != ELxItemType::Buff || pItemInfo->ItemInfo.ItemID.IsNone())
	{
		return false;
	}

	const FLxBuffDefine* BuffDefine = static_cast<const FLxBuffDefine*>(pItemInfo);

	m_BuffData = FLxBuffData();
	m_BuffData.ItemInfo = BuffDefine->ItemInfo;
	m_BuffData.ItemStackInfo = BuffDefine->ItemStackInfo;
	m_BuffData.ItemCount = 1;
	m_BuffData.ItemShowInfo = BuffDefine->ItemShowInfo;
	if (const FLxRarityInfo* RarityInfo = BuffDefine->ItemRarityRowQuote.GetRow<FLxRarityInfo>(TEXT("ULxBuffLogic")))
	{
		m_BuffData.ItemRarity = *RarityInfo;
	}
	m_BuffData.BuffCoreInfo = BuffDefine->BuffCoreInfo;
	m_BuffData.BuffEffectInfo = BuffDefine->BuffEffectInfo;
	for (const FLxItemEntryQuote& EntryQuote : BuffDefine->BuffEntryList)
	{
		if (ULxItemEntryLogic* EntryLogic = ULxItemEntryLogic::CreateItemEntryLogicObject(EntryQuote, this))
		{
			m_BuffData.BuffEntryList.Add(EntryLogic);
		}
	}

	OnItemInfoChanged.Broadcast();
	return true;
}

FLxItemDateBase* ULxBuffLogic::GetItemDataBase()
{
	return &m_BuffData;
}

bool ULxBuffLogic::UseItem()
{
	return IsBuffValid();
}

bool ULxBuffLogic::IsBuffValid() const
{
	return m_BuffData.BuffCoreInfo.BuffID != ELxBuffID::None && !m_BuffData.ItemInfo.ItemID.IsNone();
}

void ULxBuffLogic::SetEffectProportion(float InEffectProportion)
{
	m_BuffData.BuffEffectInfo.Proportion = InEffectProportion;
	for (ULxItemEntryLogic* EntryLogic : m_BuffData.BuffEntryList)
	{
		if (EntryLogic != nullptr && EntryLogic->IsEntryValid())
		{
			EntryLogic->GetMutableItemEntryData().EffectiveRatio = InEffectProportion;
		}
	}
	OnItemInfoChanged.Broadcast();
}

void ULxBuffLogic::SetDuration(float InDuration)
{
	m_BuffData.BuffEffectInfo.Duration = InDuration;
	OnItemInfoChanged.Broadcast();
}

