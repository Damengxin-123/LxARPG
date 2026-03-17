// Fill out your copyright notice in the Description page of Project Settings.


#include "LxARPGGameMode.h"

ALxARPGGameMode::ALxARPGGameMode()
{
}

void ALxARPGGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ALxARPGGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void ALxARPGGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
}

void ALxARPGGameMode::HandlePlayerDeath(AController* DeadPlayer)
{
}

APawn* ALxARPGGameMode::SpawnPlayerCharacter(AController* NewPlayer)
{
	return nullptr;
}
