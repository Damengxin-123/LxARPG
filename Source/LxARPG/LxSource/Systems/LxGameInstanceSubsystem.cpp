// Fill out your copyright notice in the Description page of Project Settings.


#include "LxGameInstanceSubsystem.h"
#include "DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "SettingSystem/LxGameSettings.h"
#include "StaticDataSystem/LxGlobalStaticDataManager.h"
#include "NavigationSystem/LxAINavigationRegistry.h"
#include "SaveSystem/LxSaveManager.h"
#include "Engine/World.h"

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
	SaveManager = NewObject<ULxSaveManager>(this, TEXT("存档管理模块"));
	const ULxGameSettings* Settings = GetDefault<ULxGameSettings>();
	SaveManager->Initialize(Settings->SaveSlotName, Settings->SaveUserIndex);
	WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::HandleWorldBeginTearDown);
	LevelRemovedHandle = FWorldDelegates::PreLevelRemovedFromWorld.AddUObject(this, &ThisClass::HandleLevelRemovedFromWorld);
	RequestLoadSave();
}

void ULxGameInstanceSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
	FWorldDelegates::PreLevelRemovedFromWorld.Remove(LevelRemovedHandle);
	PerformSave(true);
	SaveManager = nullptr;
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

bool ULxGameInstanceSubsystem::RequestLoadSave()
{
	if (!SaveManager || bSaveOperationInProgress)
	{
		return false;
	}
	TGuardValue<bool> OperationGuard(bSaveOperationInProgress, true);
	OnLoadSaveRequested.Broadcast();
	const bool bSuccess = SaveManager->LoadSave();
	OnLoadSaveFinished.Broadcast(bSuccess);
	return bSuccess;
}

bool ULxGameInstanceSubsystem::RequestSaveGame()
{
	return PerformSave(false);
}

bool ULxGameInstanceSubsystem::PerformSave(bool bCacheOnly)
{
	if (!SaveManager || SaveManager->IsReadOnly() || bSaveOperationInProgress)
	{
		return false;
	}
	TGuardValue<bool> OperationGuard(bSaveOperationInProgress, true);
	OnSaveGameRequested.Broadcast();
	const bool bSuccess = bCacheOnly ? SaveManager->SaveCachedData() : SaveManager->SaveAll();
	OnSaveGameFinished.Broadcast(bSuccess);
	return bSuccess;
}

void ULxGameInstanceSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance() && SaveManager && !SaveManager->IsReadOnly())
	{
		SaveManager->CacheWorldBeforeCleanup(World);
		PerformSave(true);
	}
}

void ULxGameInstanceSubsystem::HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	if (World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance() && SaveManager && !SaveManager->IsReadOnly())
	{
		SaveManager->CacheWorldBeforeCleanup(World, Level);
	}
}

ULxGlobalStaticDataManager* ULxGameInstanceSubsystem::GetGlobalStaticDataManager() const
{
	return GlobalStaticDataManager;
}

void ULxGameInstanceSubsystem::SetSessionSaveManager(ULxSaveManager* InManager)
{
	if (InManager && !bSaveOperationInProgress)
	{
		SaveManager = InManager;
	}
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
