// Fill out your copyright notice in the Description page of Project Settings.

#include "LxInputActionInfoTableConfig.h"

void ULxInputActionInfoTableConfig::InitDataTableLoading()
{
	m_tInputActionInfoMap.Empty();

	for (UDataTable* Table : m_vInputActionInfoTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxInputActionInfo*> Rows;
		Table->GetAllRows<FLxInputActionInfo>(TEXT("ULxInputActionInfoTableConfig"), Rows);

		for (const FLxInputActionInfo* RowData : Rows)
		{
			if (RowData == nullptr || RowData->InputActionID.IsNone())
			{
				continue;
			}

			m_tInputActionInfoMap.Add(RowData->InputActionID, *RowData);
		}
	}
}

const FLxInputActionInfo* ULxInputActionInfoTableConfig::GetInputActionInfo(const FName& InRowID) const
{
	if (InRowID.IsNone())
	{
		return nullptr;
	}

	return m_tInputActionInfoMap.Find(InRowID);
}

const TMap<FName, FLxInputActionInfo>& ULxInputActionInfoTableConfig::GetInputActionInfoMap() const
{
	return m_tInputActionInfoMap;
}
