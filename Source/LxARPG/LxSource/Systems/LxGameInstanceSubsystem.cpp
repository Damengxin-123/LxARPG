// Fill out your copyright notice in the Description page of Project Settings.


#include "LxGameInstanceSubsystem.h"
#include "DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "SettingSystem/LxGameSettings.h"

ULxGameInstanceSubsystem* ULxGameInstanceSubsystem::GetInstance(const UWorld* InWorldPtr)
{
	if (!InWorldPtr)
	{
		return nullptr;
	}
	if (UGameInstance* GI = InWorldPtr->GetGameInstance())
	{
		if (ULxGameInstanceSubsystem* GameInstanceSubsystem = GI->GetSubsystem<ULxGameInstanceSubsystem>())
		{
			return GameInstanceSubsystem;
		}
	}
	return nullptr;
}

void ULxGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadDataTables();
}

const ULxGameDataTablesManager* ULxGameInstanceSubsystem::GetGameDataManager() const
{
	if (m_vGameDataManager)
	{
		return m_vGameDataManager;
	}
	return nullptr;
}

void ULxGameInstanceSubsystem::LoadDataTables()
{
	// 创建表格管理器 GameDataTablesManagerObject
	if (const ULxGameSettings* Settings = GetDefault<ULxGameSettings>())
	{
		if (Settings->GameDataTablesManagerClass)
		{
			m_vGameDataManager = NewObject<ULxGameDataTablesManager>(this, Settings->GameDataTablesManagerClass);
		}
		else
		{
			ERROR_TO_SCREEN("LoadDataTables error! GameDataTablesManagerClass is null!");
		}
	}
	
	if (m_vGameDataManager)
	{
		m_vGameDataManager->LoadDataTables();
	}
}
