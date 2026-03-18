// Fill out your copyright notice in the Description page of Project Settings.


#include "LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Model/Input/LxInputComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Components/LxPlayerSystemOperateComponent.h"
#include "LxARPG/LxSource/Systems/GameMode/LxARPGGameMode.h"
#include "Engine/World.h"

ALxPlayerController::ALxPlayerController()
{
	m_pInputComponent = CreateDefaultSubobject<ULxInputComponent>(TEXT("外部输入管理组件"));
	m_pSystemOperateComponent = CreateDefaultSubobject<ULxPlayerSystemOperateComponent>(TEXT("系统操作组件"));

}


void ALxPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (m_pInputComponent)
	{
		if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
		{
			LocalPlayerSubsystem->SetInputComponentQuote(m_pInputComponent);
		}
	}
}

void ALxPlayerController::CreatePlayerCharacter()
{
	if (GetNetMode() == NM_Standalone)
	{
		CreateLocalPlayerCharacter();
		return;
	}

	CreateServerPlayerCharacter();
}

void ALxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
	if (m_pCurrentCharacter)
	{
		m_pCurrentCharacter->InitialCharacterInformation();
	}
}

void ALxPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
	if (m_pCurrentCharacter)
	{
		m_pCurrentCharacter->InitialCharacterInformation();
	}
}

void ALxPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	m_pCurrentCharacter = nullptr;
}

void ALxPlayerController::SyncControlledCharacter(APawn* InPawn)
{
	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
	if (m_pCurrentCharacter)
	{
		m_pCurrentCharacter->InitialCharacterInformation();
	}
}

void ALxPlayerController::CreateLocalPlayerCharacter()
{
	if (ALxARPGGameMode* GameMode = GetWorld() ? Cast<ALxARPGGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (APawn* NewPawn = GameMode->SpawnPlayerCharacter(this))
		{
			Possess(NewPawn);
		}
	}
}

void ALxPlayerController::CreateServerPlayerCharacter_Implementation()
{
	if (ALxARPGGameMode* GameMode = GetWorld() ? Cast<ALxARPGGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (APawn* NewPawn = GameMode->SpawnPlayerCharacter(this))
		{
			Possess(NewPawn);
		}
	}
}
