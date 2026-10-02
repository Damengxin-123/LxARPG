// Fill out your copyright notice in the Description page of Project Settings.


#include "LxARPGGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveComponent.h"

ALxARPGGameMode::ALxARPGGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = true;
}

void ALxARPGGameMode::StartPlay()
{
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (Menu && Menu->IsEnteringGame())
	{
		bWaitingForMenuSession = true;
		Menu->PrepareGameplayWorld(GetWorld());
		return;
	}
	Super::StartPlay();
}

void ALxARPGGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bWaitingForMenuSession) return;
	ULxMainMenuSubsystem* Menu = GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>();
	if (!Menu->IsGameplayWorldReady()) return;
	bWaitingForMenuSession = false;
	bSessionSpawnAllowed = true;
	Super::StartPlay();
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (Controller && !Controller->GetPawn()) RestartPlayer(Controller);
	Menu->CompleteGameplayStart(Controller);
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
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (Menu && Menu->HasSession())
	{
		if (!NewPlayer || (Menu->IsEnteringGame() && !bSessionSpawnAllowed) || NewPlayer->GetPawn()) return;
		if (APawn* Pawn = SpawnPlayerCharacter(NewPlayer)) NewPlayer->Possess(Pawn);
		return;
	}
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
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	const FLxCharacterSaveRecord* Session = Menu ? Menu->GetSessionRecord() : nullptr;
	if (Session)
	{
		if (Menu->IsEnteringGame() && !bSessionSpawnAllowed) return nullptr;
		if (NewPlayer->GetPawn()) return NewPlayer->GetPawn();
		SpawnClass = Session->CharacterClass.LoadSynchronous();
	}
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

	FTransform SpawnTransform = FirstPlayerStart
		? FirstPlayerStart->GetActorTransform()
		: FTransform::Identity;
	if (Session) SpawnTransform = Menu->GetSafeSpawnTransform(GetWorld(), SpawnClass);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = NewPlayer;
	SpawnParams.Instigator = NewPlayer->GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.bDeferConstruction = Session != nullptr;

	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(SpawnClass, SpawnTransform, SpawnParams);
	if (!NewPawn)
	{
		return nullptr;
	}
	if (Session)
	{
		NewPawn->AutoPossessPlayer = EAutoReceiveInput::Disabled;
		if (ULxCharacterSaveComponent* Save = NewPawn->FindComponentByClass<ULxCharacterSaveComponent>()) Save->SetSessionIdentity(Session->SaveID);
		UGameplayStatics::FinishSpawningActor(NewPawn, SpawnTransform);
	}

	return NewPawn;
}
