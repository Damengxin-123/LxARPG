// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LxGameSettings.generated.h"

class ULxGameDataTablesManager;
class ULxUIManager;

UCLASS(config=Game, defaultconfig, DisplayName="Game Settings")
class LXARPG_API ULxGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, config, Category="Data|ManagerClass", DisplayName="Game Data Tables Manager Class")
	TSubclassOf<ULxGameDataTablesManager> GameDataTablesManagerClass;

	UPROPERTY(EditDefaultsOnly, config, Category="UI|ManagerClass", DisplayName="UI Manager Class")
	TSubclassOf<ULxUIManager> UIManagerClass;
};
