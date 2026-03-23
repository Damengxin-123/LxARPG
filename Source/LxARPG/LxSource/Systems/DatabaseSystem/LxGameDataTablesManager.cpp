// Fill out your copyright notice in the Description page of Project Settings.


#include "LxGameDataTablesManager.h"
#include "LxDataTable.h"

ULxDataTable* ULxGameDataTablesManager::GetDataTables(const ELxDataTableTypeEnum InDataTableType) const
{
	if (m_mapTablesMap.Contains(InDataTableType))
	{
		return m_mapTablesMap[InDataTableType];
	}
	return nullptr;
}

bool ULxGameDataTablesManager::TableIsLoadingCompleted(const ELxDataTableTypeEnum InDataTableType) const
{
	if (m_mapTablesMap.Contains(InDataTableType))
	{
		return m_mapTablesMap[InDataTableType] != nullptr;
	}
	return false;
} 

void ULxGameDataTablesManager::LoadDataTables()
{
	ULxDataTable* table = nullptr;
	for (auto& tableName : m_mapTablesSetting)
	{
		if (tableName.Value)
		{
			table = NewObject<ULxDataTable>(this, tableName.Value);
			if (table)
			{
				table->LoadDataTables(tableName.Key, TEXT("LoadDataTables"));
				m_mapTablesMap.Add(tableName.Key, table);
			}
			table = nullptr;
		}
	}

}
