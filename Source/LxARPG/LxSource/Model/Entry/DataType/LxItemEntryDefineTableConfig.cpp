// Fill out your copyright notice in the Description page of Project Settings.

#include "LxItemEntryDefineTableConfig.h"

void ULxItemEntryDefineTableConfig::InitDataTableLoading()
{
	m_tItemEntryDefineMap.Empty();

	for (UDataTable* Table : m_vItemEntryDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxItemEntryDefine*> Rows;
		Table->GetAllRows<FLxItemEntryDefine>(TEXT("ULxItemEntryDefineTableConfig"), Rows);

		for (const FLxItemEntryDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->EnteryBaseInfo.EntryID.IsNone())
			{
				continue;
			}

			m_tItemEntryDefineMap.Add(RowData->EnteryBaseInfo.EntryID, *RowData);
		}
	}
}

const FLxItemEntryDefine* ULxItemEntryDefineTableConfig::GetItemEntryDefine(const FName& InEntryID) const
{
	if (InEntryID.IsNone())
	{
		return nullptr;
	}

	return m_tItemEntryDefineMap.Find(InEntryID);
}
