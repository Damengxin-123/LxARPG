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

FLxItemDateBase* ULxSkillLogic::GetItemDataBase()
{
	return &m_SkillData;
}

bool ULxSkillLogic::UseItem()
{
	return false;
}




