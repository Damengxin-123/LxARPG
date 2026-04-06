// Fill out your copyright notice in the Description page of Project Settings.

#include "LxEquipmentDefineTableConfig.h"

void ULxEquipmentDefineTableConfig::InitDataTableLoading()
{
	m_tEquipmentDefineMap.Empty();

	for (UDataTable* Table : m_vEquipmentDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxEquipmentDefine*> Rows;
		Table->GetAllRows<FLxEquipmentDefine>(TEXT("ULxEquipmentDefineTableConfig"), Rows);

		for (const FLxEquipmentDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->ItemInfo.ItemID.IsNone())
			{
				continue;
			}

			m_tEquipmentDefineMap.Add(RowData->ItemInfo.ItemID, *RowData);
		}
	}
}

const FLxEquipmentDefine* ULxEquipmentDefineTableConfig::GetEquipmentDefine(const FName& InItemID) const
{
	if (InItemID.IsNone())
	{
		return nullptr;
	}

	return m_tEquipmentDefineMap.Find(InItemID);
}

