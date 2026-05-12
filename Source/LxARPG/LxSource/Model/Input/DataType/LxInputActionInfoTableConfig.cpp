// Fill out your copyright notice in the Description page of Project Settings.

#include "LxInputActionInfoTableConfig.h"
#include "LxInputActionConfig.h"

void ULxInputActionInfoTableConfig::InitDataTableLoading()
{
	m_tInputActionInfoMap.Empty();
	LxInputActionConfig::ClearInputActionConfig();

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
			if (RowData == nullptr || RowData->InputActionID == ELxInputActionID::None)
			{
				continue;
			}

			m_tInputActionInfoMap.Add(RowData->InputActionID, *RowData);
			LxInputActionConfig::SetInputActionInfo(*RowData);
		}
	}
}

const FLxInputActionInfo* ULxInputActionInfoTableConfig::GetInputActionInfo(ELxInputActionID InInputActionID) const
{
	if (InInputActionID == ELxInputActionID::None)
	{
		return nullptr;
	}

	return m_tInputActionInfoMap.Find(InInputActionID);
}

const TMap<ELxInputActionID, FLxInputActionInfo>& ULxInputActionInfoTableConfig::GetInputActionInfoMap() const
{
	return m_tInputActionInfoMap;
}
