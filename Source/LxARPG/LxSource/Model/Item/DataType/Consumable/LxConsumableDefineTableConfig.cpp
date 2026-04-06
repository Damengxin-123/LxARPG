// Fill out your copyright notice in the Description page of Project Settings.

#include "LxConsumableDefineTableConfig.h"

void ULxConsumableDefineTableConfig::InitDataTableLoading()
{
	m_tConsumableDefineMap.Empty();

	for (UDataTable* Table : m_vConsumableDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxConsumableDefine*> Rows;
		Table->GetAllRows<FLxConsumableDefine>(TEXT("ULxConsumableDefineTableConfig"), Rows);

		for (const FLxConsumableDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->ItemInfo.ItemID.IsNone())
			{
				continue;
			}

			m_tConsumableDefineMap.Add(RowData->ItemInfo.ItemID, *RowData);
		}
	}
}

const FLxConsumableDefine* ULxConsumableDefineTableConfig::GetConsumableDefine(const FName& InItemID) const
{
	if (InItemID.IsNone())
	{
		return nullptr;
	}

	return m_tConsumableDefineMap.Find(InItemID);
}
