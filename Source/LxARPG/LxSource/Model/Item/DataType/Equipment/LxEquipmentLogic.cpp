// Fill out your copyright notice in the Description page of Project Settings.


#include "LxEquipmentLogic.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxTextLineStyleData.h"


bool ULxEquipmentLogic::InitItemLogic(const FLxItemDefineBase* pItemInfo)
{
	// 基础校验：必须有输入数据
	if (pItemInfo == nullptr)
	{
		return false;
	}

	// 仅处理装备类型，避免把其他物品定义误当装备解
	if (pItemInfo->ItemInfo.ItemType != ELxItemType::Equipment || pItemInfo->ItemInfo.ItemID.IsNone())
	{
		return false;
	}

	// 按约定将基类定义解释为装备定
	const FLxEquipmentDefine* EquipmentDefine = static_cast<const FLxEquipmentDefine*>(pItemInfo);

	// 清空并重建运行时装备缓存
	m_EquipmentData = FLxEquipmentData();
	m_EquipmentData.ItemInfo = EquipmentDefine->ItemInfo;
	m_EquipmentData.ItemStackInfo = EquipmentDefine->ItemStackInfo;
	m_EquipmentData.ItemCount = 1;
	m_EquipmentData.ItemShowInfo = EquipmentDefine->ItemShowInfo;
	// 解析稀有度引用
	if (const FLxRarityInfo* RarityInfo = EquipmentDefine->ItemRarityRowQuote.GetRow<FLxRarityInfo>(TEXT("ULxEquipmentLogic")))
	{
		m_EquipmentData.ItemRarity = *RarityInfo;
	}
	m_EquipmentData.EquipmentInfo = EquipmentDefine->EquipmentInfo;

	// 构建基础词条与扩展词条缓
	m_EquipmentData.EquipmentEntyInfo.EquipmentExtendEntryList.Empty();
	BuildItemEntryData(EquipmentDefine->EquipmentEntyQuoteInfo.EquipmentBasicEntryQuote,
		m_EquipmentData.EquipmentEntyInfo.EquipmentBasicEntry);

	for (const FLxItemEntryQuote& EntryQuote : EquipmentDefine->EquipmentEntyQuoteInfo.EquipmentExtendEntryQuote)
	{
		FLxItemEntryData EntryData;
		if (BuildItemEntryData(EntryQuote, EntryData))
		{
			m_EquipmentData.EquipmentEntyInfo.EquipmentExtendEntryList.Add(EntryData);
		}
	}

	OnItemInfoChanged.Broadcast();
	return true;
}

FLxItemDateBase* ULxEquipmentLogic::GetItemDataBase()
{
	// 统一通过 m_EquipmentData 向外提供数据
	return &m_EquipmentData;
}

bool ULxEquipmentLogic::UseItem()
{
	// 装备类型本身不走“消耗使用”逻辑
	return false;
}

FLxEquipmentData* ULxEquipmentLogic::GetEquipmentData()
{
	return &m_EquipmentData;
}





