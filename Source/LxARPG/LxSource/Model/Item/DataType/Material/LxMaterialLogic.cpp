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

const FLxItemDateBase* ULxMaterialLogic::GetItemDataBase() const
{
	return &m_MaterialData;
}

bool ULxMaterialLogic::UseItem()
{
	return false;
}

bool ULxMaterialLogic::ItemIsStack()
{
	return m_MaterialData.ItemStackInfo.ItemCanStack;
}

bool ULxMaterialLogic::StackItem(ULxItemLogicBase* SourceItemLogic)
{
	if (SourceItemLogic == nullptr || SourceItemLogic == this)
	{
		return false;
	}

	if (!ItemIsStack())
	{
		return false;
	}

	ULxMaterialLogic* SourceMaterialLogic = Cast<ULxMaterialLogic>(SourceItemLogic);
	if (SourceMaterialLogic == nullptr)
	{
		return false;
	}

	if (m_MaterialData.ItemInfo.ItemID != SourceMaterialLogic->m_MaterialData.ItemInfo.ItemID)
	{
		return false;
	}

	const int32 MaxCount = m_MaterialData.ItemStackInfo.ItemMaxCount;
	const int32 Remaining = MaxCount - m_MaterialData.ItemCount;
	if (Remaining <= 0)
	{
		return false;
	}

	const int32 MoveCount = FMath::Min(Remaining, SourceMaterialLogic->m_MaterialData.ItemCount);
	if (MoveCount <= 0)
	{
		return false;
	}

	m_MaterialData.ItemCount += MoveCount;
	SourceMaterialLogic->m_MaterialData.ItemCount -= MoveCount;

	OnItemInfoChanged.Broadcast();
	SourceMaterialLogic->OnItemInfoChanged.Broadcast();
	return true;
}

bool ULxMaterialLogic::ItemIsValid()
{
	return m_MaterialData.ItemInfo.ItemType == ELxItemType::Material
		&& !m_MaterialData.ItemInfo.ItemID.IsNone()
		&& m_MaterialData.ItemCount > 0
		&& m_MaterialData.ItemCount != ERR_ATTRIBUTE;
}

bool ULxMaterialLogic::operator<(const ULxItemLogicBase* Other) const
{
	const ULxMaterialLogic* OtherMaterial = Cast<ULxMaterialLogic>(Other);
	if (OtherMaterial == nullptr)
	{
		return false;
	}

	if (m_MaterialData.ItemRarity.RarityValue != OtherMaterial->m_MaterialData.ItemRarity.RarityValue)
	{
		return m_MaterialData.ItemRarity.RarityValue < OtherMaterial->m_MaterialData.ItemRarity.RarityValue;
	}

	return m_MaterialData.ItemInfo.ItemID.LexicalLess(OtherMaterial->m_MaterialData.ItemInfo.ItemID);
}

bool ULxMaterialLogic::operator>(const ULxItemLogicBase* Other) const
{
	const ULxMaterialLogic* OtherMaterial = Cast<ULxMaterialLogic>(Other);
	if (OtherMaterial == nullptr)
	{
		return false;
	}

	if (m_MaterialData.ItemRarity.RarityValue != OtherMaterial->m_MaterialData.ItemRarity.RarityValue)
	{
		return m_MaterialData.ItemRarity.RarityValue > OtherMaterial->m_MaterialData.ItemRarity.RarityValue;
	}

	return OtherMaterial->m_MaterialData.ItemInfo.ItemID.LexicalLess(m_MaterialData.ItemInfo.ItemID);
}
