// Fill out your copyright notice in the Description page of Project Settings.

#include "LxMaterialDefineTableConfig.h"

void ULxMaterialDefineTableConfig::InitDataTableLoading()
{
	m_tMaterialDefineMap.Empty();

	for (UDataTable* Table : m_vMaterialDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxMaterialDefine*> Rows;
		Table->GetAllRows<FLxMaterialDefine>(TEXT("ULxMaterialDefineTableConfig"), Rows);

		for (const FLxMaterialDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->ItemInfo.ItemID.IsNone())
			{
				continue;
			}

			m_tMaterialDefineMap.Add(RowData->ItemInfo.ItemID, *RowData);
		}
	}
}

const FLxMaterialDefine* ULxMaterialDefineTableConfig::GetMaterialDefine(const FName& InItemID) const
{
	if (InItemID.IsNone())
	{
		return nullptr;
	}

	return m_tMaterialDefineMap.Find(InItemID);
}
