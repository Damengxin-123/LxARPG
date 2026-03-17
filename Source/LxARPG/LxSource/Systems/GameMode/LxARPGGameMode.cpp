// Fill out your copyright notice in the Description page of Project Settings.


#include "LxARPGGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

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
	if (!NewPlayer)
	{
		return nullptr;
	}

	UClass* SpawnClass = DefaultPlayerPawnClass ? *DefaultPlayerPawnClass : GetDefaultPawnClassForController(NewPlayer);
	if (!SpawnClass)
	{
		return nullptr;
	}

	APlayerStart* FirstPlayerStart = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		FirstPlayerStart = *It;
		break;
	}

	const FTransform SpawnTransform = FirstPlayerStart
		? FirstPlayerStart->GetActorTransform()
		: FTransform::Identity;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = NewPlayer;
	SpawnParams.Instigator = NewPlayer->GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(SpawnClass, SpawnTransform, SpawnParams);
	if (!NewPawn)
	{
		return nullptr;
	}

	NewPlayer->SetPawn(NewPawn);
	return NewPawn;
}
