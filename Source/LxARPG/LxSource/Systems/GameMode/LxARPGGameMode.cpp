// Fill out your copyright notice in the Description page of Project Settings.


#include "LxARPGGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSubsystem.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveComponent.h"

ALxARPGGameMode::ALxARPGGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = true;
}

void ALxARPGGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	bShowingMainMenu = GetNetMode() != NM_DedicatedServer && Menu && !Menu->HasSession();
	if (bShowingMainMenu)
	{
		if (ULxGameInstanceSubsystem* Global = ULxGameInstanceSubsystem::GetInstance(GetWorld()))
		{
			if (ULxSaveManager* Saves = Global->GetSaveManager()) Saves->SetReadOnly(true);
		}
	}
}

bool ALxARPGGameMode::BeginMenuSession()
{
	UWorld* World = GetWorld();
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (!bShowingMainMenu || !World || World->HasBegunPlay() || !Menu || !Menu->IsEnteringGame()) return false;
	bShowingMainMenu = false;
	bWaitingForMenuSession = true;
	Menu->PrepareGameplayWorld(World);
	return true;
}

void ALxARPGGameMode::StartPlay()
{
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (bShowingMainMenu)
	{
		if (Menu) Menu->ShowMenu(GetWorld());
		return;
	}
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
	ULxMainMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULxMainMenuSubsystem>() : nullptr;
	if (bShowingMainMenu)
	{
		if (Menu) Menu->TickPreview(DeltaSeconds);
		return;
	}
	if (!bWaitingForMenuSession || !Menu || !Menu->IsGameplayWorldReady()) return;
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
	if (bShowingMainMenu) return;
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
	if (bShowingMainMenu || !NewPlayer)
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
		if (ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(NewPawn)) Player->SetCharacterRaceForSession(Session->CharacterRace);
		if (ULxCharacterSaveComponent* Save = NewPawn->FindComponentByClass<ULxCharacterSaveComponent>()) Save->SetSessionIdentity(Session->SaveID);
		UGameplayStatics::FinishSpawningActor(NewPawn, SpawnTransform);
	}

	return NewPawn;
}
