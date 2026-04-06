// Fill out your copyright notice in the Description page of Project Settings.

#include "LxBuffDefineTableConfig.h"

void ULxBuffDefineTableConfig::InitDataTableLoading()
{
	m_tBuffDefineMap.Empty();

	for (UDataTable* Table : m_vBuffDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FBuffDefine*> Rows;
		Table->GetAllRows<FBuffDefine>(TEXT("ULxBuffDefineTableConfig"), Rows);

		for (const FBuffDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->ItemInfo.ItemID.IsNone())
			{
				continue;
			}

			m_tBuffDefineMap.Add(RowData->ItemInfo.ItemID, *RowData);
		}
	}
}

const FBuffDefine* ULxBuffDefineTableConfig::GetBuffDefine(const FName& InItemID) const
{
	if (InItemID.IsNone())
	{
		return nullptr;
	}

	return m_tBuffDefineMap.Find(InItemID);
}
