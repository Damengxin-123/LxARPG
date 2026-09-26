// Fill out your copyright notice in the Description page of Project Settings.


#include "LxGameInstanceSubsystem.h"
#include "DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "SettingSystem/LxGameSettings.h"
#include "StaticDataSystem/LxGlobalStaticDataManager.h"
#include "NavigationSystem/LxAINavigationRegistry.h"

ULxGameInstanceSubsystem* ULxGameInstanceSubsystem::GetInstance(const UWorld* InWorldPtr)
{
	if (!InWorldPtr)
	{
		return nullptr;
	}
	if (UGameInstance* GI = InWorldPtr->GetGameInstance())
	{
		return GI->GetSubsystem<ULxGameInstanceSubsystem>();
	}
	return nullptr;
}

void ULxGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadDataTables();
	InitializeGlobalStaticDataManager();
	AINavigationRegistry = NewObject<ULxAINavigationRegistry>(this, TEXT("AI导航注册表"));
}

void ULxGameInstanceSubsystem::Deinitialize()
{
	if (AINavigationRegistry)
	{
		AINavigationRegistry->Deinitialize();
		AINavigationRegistry = nullptr;
	}
	if (GlobalStaticDataManager)
	{
		GlobalStaticDataManager->Deinitialize();
		GlobalStaticDataManager = nullptr;
	}
	m_vGameDataManager = nullptr;

	Super::Deinitialize();
}

const ULxGameDataTablesManager* ULxGameInstanceSubsystem::GetGameDataManager() const
{
	return m_vGameDataManager;
}

ULxGlobalStaticDataManager* ULxGameInstanceSubsystem::GetGlobalStaticDataManager() const
{
	return GlobalStaticDataManager;
}

ULxAINavigationRegistry* ULxGameInstanceSubsystem::GetAINavigationRegistry() const
{
	return AINavigationRegistry;
}

void ULxGameInstanceSubsystem::InitializeGlobalStaticDataManager()
{
	GlobalStaticDataManager = NewObject<ULxGlobalStaticDataManager>(this, TEXT("全局静态数据管理器"));
	GlobalStaticDataManager->Initialize(m_vGameDataManager.Get());
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
