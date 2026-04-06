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

const FLxItemDateBase* ULxBuffLogic::GetItemDataBase() const
{
	return &m_BuffData;
}

bool ULxBuffLogic::UseItem()
{
	return false;
}

bool ULxBuffLogic::ItemIsStack()
{
	return m_BuffData.ItemStackInfo.ItemCanStack;
}

bool ULxBuffLogic::StackItem(ULxItemLogicBase* SourceItemLogic)
{
	if (SourceItemLogic == nullptr || SourceItemLogic == this)
	{
		return false;
	}

	if (!ItemIsStack())
	{
		return false;
	}

	ULxBuffLogic* SourceBuffLogic = Cast<ULxBuffLogic>(SourceItemLogic);
	if (SourceBuffLogic == nullptr)
	{
		return false;
	}

	if (m_BuffData.ItemInfo.ItemID != SourceBuffLogic->m_BuffData.ItemInfo.ItemID)
	{
		return false;
	}

	const int32 MaxCount = m_BuffData.ItemStackInfo.ItemMaxCount;
	const int32 Remaining = MaxCount - m_BuffData.ItemCount;
	if (Remaining <= 0)
	{
		return false;
	}

	const int32 MoveCount = FMath::Min(Remaining, SourceBuffLogic->m_BuffData.ItemCount);
	if (MoveCount <= 0)
	{
		return false;
	}

	m_BuffData.ItemCount += MoveCount;
	SourceBuffLogic->m_BuffData.ItemCount -= MoveCount;

	OnItemInfoChanged.Broadcast();
	SourceBuffLogic->OnItemInfoChanged.Broadcast();
	return true;
}

bool ULxBuffLogic::ItemIsValid()
{
	return m_BuffData.ItemInfo.ItemType == ELxItemType::Buff
		&& !m_BuffData.ItemInfo.ItemID.IsNone()
		&& m_BuffData.ItemCount > 0
		&& m_BuffData.ItemCount != ERR_ATTRIBUTE;
}

bool ULxBuffLogic::operator<(const ULxItemLogicBase* Other) const
{
	const ULxBuffLogic* OtherBuff = Cast<ULxBuffLogic>(Other);
	if (OtherBuff == nullptr)
	{
		return false;
	}

	if (m_BuffData.ItemRarity.RarityValue != OtherBuff->m_BuffData.ItemRarity.RarityValue)
	{
		return m_BuffData.ItemRarity.RarityValue < OtherBuff->m_BuffData.ItemRarity.RarityValue;
	}

	return m_BuffData.ItemInfo.ItemID.LexicalLess(OtherBuff->m_BuffData.ItemInfo.ItemID);
}

bool ULxBuffLogic::operator>(const ULxItemLogicBase* Other) const
{
	const ULxBuffLogic* OtherBuff = Cast<ULxBuffLogic>(Other);
	if (OtherBuff == nullptr)
	{
		return false;
	}

	if (m_BuffData.ItemRarity.RarityValue != OtherBuff->m_BuffData.ItemRarity.RarityValue)
	{
		return m_BuffData.ItemRarity.RarityValue > OtherBuff->m_BuffData.ItemRarity.RarityValue;
	}

	return OtherBuff->m_BuffData.ItemInfo.ItemID.LexicalLess(m_BuffData.ItemInfo.ItemID);
}
