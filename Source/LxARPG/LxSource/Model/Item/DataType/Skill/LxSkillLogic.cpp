// Fill out your copyright notice in the Description page of Project Settings.

#include "LxSkillLogic.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"

bool ULxSkillLogic::InitItemLogic(const FLxItemDefineBase* pItemInfo)
{
	if (pItemInfo == nullptr)
	{
		return false;
	}

	if (pItemInfo->ItemInfo.ItemType != ELxItemType::Skill || pItemInfo->ItemInfo.ItemID.IsNone())
	{
		return false;
	}

	const FLxSkillDefine* SkillDefine = static_cast<const FLxSkillDefine*>(pItemInfo);

	m_SkillData = FLxSkilllData();
	m_SkillData.ItemInfo = SkillDefine->ItemInfo;
	m_SkillData.ItemStackInfo = SkillDefine->ItemStackInfo;
	m_SkillData.ItemCount = 1;
	m_SkillData.ItemShowInfo = SkillDefine->ItemShowInfo;
	if (const FLxRarityInfo* RarityInfo = SkillDefine->ItemRarityRowQuote.GetRow<FLxRarityInfo>(TEXT("ULxSkillLogic")))
	{
		m_SkillData.ItemRarity = *RarityInfo;
	}
	m_SkillData.SkillCoreInfo = SkillDefine->SkillCoreInfo;

	OnItemInfoChanged.Broadcast();
	return true;
}

const FLxItemDateBase* ULxSkillLogic::GetItemDataBase() const
{
	return &m_SkillData;
}

bool ULxSkillLogic::UseItem()
{
	return false;
}

bool ULxSkillLogic::ItemIsStack()
{
	return m_SkillData.ItemStackInfo.ItemCanStack;
}

bool ULxSkillLogic::StackItem(ULxItemLogicBase* SourceItemLogic)
{
	if (SourceItemLogic == nullptr || SourceItemLogic == this)
	{
		return false;
	}

	if (!ItemIsStack())
	{
		return false;
	}

	ULxSkillLogic* SourceSkillLogic = Cast<ULxSkillLogic>(SourceItemLogic);
	if (SourceSkillLogic == nullptr)
	{
		return false;
	}

	if (m_SkillData.ItemInfo.ItemID != SourceSkillLogic->m_SkillData.ItemInfo.ItemID)
	{
		return false;
	}

	const int32 MaxCount = m_SkillData.ItemStackInfo.ItemMaxCount;
	const int32 Remaining = MaxCount - m_SkillData.ItemCount;
	if (Remaining <= 0)
	{
		return false;
	}

	const int32 MoveCount = FMath::Min(Remaining, SourceSkillLogic->m_SkillData.ItemCount);
	if (MoveCount <= 0)
	{
		return false;
	}

	m_SkillData.ItemCount += MoveCount;
	SourceSkillLogic->m_SkillData.ItemCount -= MoveCount;

	OnItemInfoChanged.Broadcast();
	SourceSkillLogic->OnItemInfoChanged.Broadcast();
	return true;
}

bool ULxSkillLogic::ItemIsValid()
{
	return m_SkillData.ItemInfo.ItemType == ELxItemType::Skill
		&& !m_SkillData.ItemInfo.ItemID.IsNone()
		&& m_SkillData.ItemCount > 0
		&& m_SkillData.ItemCount != ERR_ATTRIBUTE;
}

bool ULxSkillLogic::operator<(const ULxItemLogicBase* Other) const
{
	const ULxSkillLogic* OtherSkill = Cast<ULxSkillLogic>(Other);
	if (OtherSkill == nullptr)
	{
		return false;
	}

	if (m_SkillData.ItemRarity.RarityValue != OtherSkill->m_SkillData.ItemRarity.RarityValue)
	{
		return m_SkillData.ItemRarity.RarityValue < OtherSkill->m_SkillData.ItemRarity.RarityValue;
	}

	return m_SkillData.ItemInfo.ItemID.LexicalLess(OtherSkill->m_SkillData.ItemInfo.ItemID);
}

bool ULxSkillLogic::operator>(const ULxItemLogicBase* Other) const
{
	const ULxSkillLogic* OtherSkill = Cast<ULxSkillLogic>(Other);
	if (OtherSkill == nullptr)
	{
		return false;
	}

	if (m_SkillData.ItemRarity.RarityValue != OtherSkill->m_SkillData.ItemRarity.RarityValue)
	{
		return m_SkillData.ItemRarity.RarityValue > OtherSkill->m_SkillData.ItemRarity.RarityValue;
	}

	return OtherSkill->m_SkillData.ItemInfo.ItemID.LexicalLess(m_SkillData.ItemInfo.ItemID);
}
