// Fill out your copyright notice in the Description page of Project Settings.

#include "LxBuffDefineTableConfig.h"

void ULxBuffDefineTableConfig::InitDataTableLoading()
{
	m_tBuffDefineMap.Empty();
	m_tBuffIDDefineMap.Empty();

	for (UDataTable* Table : m_vBuffDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxBuffDefine*> Rows;
		Table->GetAllRows<FLxBuffDefine>(TEXT("ULxBuffDefineTableConfig"), Rows);

		for (const FLxBuffDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->ItemInfo.ItemID.IsNone())
			{
				continue;
			}

			m_tBuffDefineMap.Add(RowData->ItemInfo.ItemID, *RowData);
			if (RowData->BuffCoreInfo.BuffID != ELxBuffID::None)
			{
				m_tBuffIDDefineMap.Add(RowData->BuffCoreInfo.BuffID, *RowData);
			}
		}
	}
}

const FLxBuffDefine* ULxBuffDefineTableConfig::GetBuffDefine(const FName& InItemID) const
{
	if (InItemID.IsNone())
	{
		return nullptr;
	}

	return m_tBuffDefineMap.Find(InItemID);
}

const FLxBuffDefine* ULxBuffDefineTableConfig::GetBuffDefineByBuffID(ELxBuffID InBuffID) const
{
	if (InBuffID == ELxBuffID::None)
	{
		return nullptr;
	}

	return m_tBuffIDDefineMap.Find(InBuffID);
}
