// Fill out your copyright notice in the Description page of Project Settings.

#include "LxBuffLogic.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"

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

	const FBuffDefine* BuffDefine = static_cast<const FBuffDefine*>(pItemInfo);

	m_BuffData = FBuffData();
	m_BuffData.ItemInfo = BuffDefine->ItemInfo;
	m_BuffData.ItemStackInfo = BuffDefine->ItemStackInfo;
	m_BuffData.ItemCount = 1;
	m_BuffData.ItemShowInfo = BuffDefine->ItemShowInfo;
	if (const FLxRarityInfo* RarityInfo = BuffDefine->ItemRarityRowQuote.GetRow<FLxRarityInfo>(TEXT("ULxBuffLogic")))
	{
		m_BuffData.ItemRarity = *RarityInfo;
	}
	m_BuffData.BuffCoreInfo = BuffDefine->BuffCoreInfo;

	OnItemInfoChanged.Broadcast();
	return true;
}

FLxItemDateBase* ULxBuffLogic::GetItemDataBase()
{
	return &m_BuffData;
}

bool ULxBuffLogic::UseItem()
{
	return false;
}




