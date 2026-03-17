// Fill out your copyright notice in the Description page of Project Settings.


#include "LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/Model/Input/LxInputComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ALxPlayerController::ALxPlayerController()
{
	m_pInputComponent = CreateDefaultSubobject<ULxInputComponent>(TEXT("外部输入管理组件"));
	// 将输入组件注册到本地系统中
	if (m_pInputComponent)
	{
		if (ULxLocalPlayerSubsystem* LocalPlayerSubsystem = GET_LOCAL_PLAYER_SYSTEM())
		{
			LocalPlayerSubsystem->SetInputComponentQuote(m_pInputComponent);
		}
	}
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
	
}

void ALxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
}

void ALxPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
}

void ALxPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

void ALxPlayerController::SyncControlledCharacter(APawn* InPawn)
{
	m_pCurrentCharacter =  Cast<ALxBaseCharacter>(InPawn);
}

void ALxPlayerController::CreateLocalPlayerCharacter()
{
}

void ALxPlayerController::CreateServerPlayerCharacter_Implementation()
{
}
