// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LxGameSettings.generated.h"
class ULxGameDataTablesManager;
/**
 * @brief 游戏设置类，继承自UDeveloperSettings。
 */
UCLASS(config=Game, defaultconfig, DisplayName="游戏类型设置")
class LXARPG_API ULxGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:	
	// 数据表格管理类型
	UPROPERTY(EditDefaultsOnly, config, Category="数据管理|数据表格管理类型", DisplayName="数据表格管理类型")
	TSubclassOf<ULxGameDataTablesManager> GameDataTablesManagerClass;
};
