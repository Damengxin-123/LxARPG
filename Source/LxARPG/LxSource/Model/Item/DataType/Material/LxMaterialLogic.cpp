// Fill out your copyright notice in the Description page of Project Settings.

#include "LxMaterialLogic.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"

bool ULxMaterialLogic::InitItemLogic(const FLxItemDefineBase* pItemInfo)
{
	if (pItemInfo == nullptr)
	{
		return false;
	}

	if (pItemInfo->ItemInfo.ItemType != ELxItemType::Material || pItemInfo->ItemInfo.ItemID.IsNone())
	{
		return false;
	}

	const FLxMaterialDefine* MaterialDefine = static_cast<const FLxMaterialDefine*>(pItemInfo);

	m_MaterialData = FLxMaterialData();
	m_MaterialData.ItemInfo = MaterialDefine->ItemInfo;
	m_MaterialData.ItemStackInfo = MaterialDefine->ItemStackInfo;
	m_MaterialData.ItemCount = 1;
	m_MaterialData.ItemShowInfo = MaterialDefine->ItemShowInfo;
	if (const FLxRarityInfo* RarityInfo = MaterialDefine->ItemRarityRowQuote.GetRow<FLxRarityInfo>(TEXT("ULxMaterialLogic")))
	{
		m_MaterialData.ItemRarity = *RarityInfo;
	}
	m_MaterialData.MaterialCoreInfo = MaterialDefine->MaterialCoreInfo;

	OnItemInfoChanged.Broadcast();
	return true;
}

FLxItemDateBase* ULxMaterialLogic::GetItemDataBase()
{
	return &m_MaterialData;
}

bool ULxMaterialLogic::UseItem()
{
	return false;
}



