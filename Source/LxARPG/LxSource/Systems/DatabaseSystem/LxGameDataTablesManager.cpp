// Fill out your copyright notice in the Description page of Project Settings.


#include "LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"

ULxDataTableConfigBase* ULxGameDataTablesManager::GetDataTables(const ELxDataTableTypeEnum InDataTableType) const
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
	ULxDataTableConfigBase* table = nullptr;
	for (auto& tableName : m_mapTablesSetting)
	{
		if (tableName.Value)
		{
			table = NewObject<ULxDataTableConfigBase>(this, tableName.Value);
			if (table)
			{
				table->InitDataTableLoading();
				m_mapTablesMap.Add(tableName.Key, table);
			}
			table = nullptr;
		}
	}

}
