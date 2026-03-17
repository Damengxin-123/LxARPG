// Fill out your copyright notice in the Description page of Project Settings.


#include "LxGameDataTablesManager.h"
#include "LxDataTable.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Input/LxInputData.h"

ULxDataTable* ULxGameDataTablesManager::GetDataTables(const ELxDataTableTypeEnum InDataTableType) const
{
	if (m_mapTablesMap.Contains(InDataTableType))
	{
		return m_mapTablesMap[InDataTableType];
	}
	// switch (InDataTableType)
	// {
	// case ELxDataTableTypeEnum::InputActionInfo:
	// 	return InputDataBase;
	// }
	return nullptr;
}

bool ULxGameDataTablesManager::TableIsLoadingCompleted(const ELxDataTableTypeEnum InDataTableType) const
{
	if (m_mapTablesMap.Contains(InDataTableType))
	{
		return m_mapTablesMap[InDataTableType] != nullptr;
	}
	// switch (InDataTableType)
	// {
	// 	case ELxDataTableTypeEnum::InputActionInfo:
	// 		return InputDataBase != nullptr;
	// }
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
	//
	//
	// if (InputDataBase)
	// {
	// 	InputDataBase->LoadDataTables(ELxDataTableTypeEnum::InputActionInfo, TEXT("InputActionInfo"));
	// }
}
