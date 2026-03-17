// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LxGameInstanceSubsystem.generated.h"

class ULxGameDataTablesManager;
/**
 * 
 */
UCLASS()
class LXARPG_API ULxGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static ULxGameInstanceSubsystem* GetInstance(const UWorld* InWorldPtr);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const ULxGameDataTablesManager* GetGameDataManager() const;
private:

	void LoadDataTables();
	
	// 数据表管理对象
	UPROPERTY()
	TObjectPtr<ULxGameDataTablesManager> m_vGameDataManager;
};
