// Fill out your copyright notice in the Description page of Project Settings.


#include "LxLocalPlayerSubsystem.h"

#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

void ULxLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const ULxGameSettings* GameSettings = GetDefault<ULxGameSettings>();
	if (!GameSettings || !GameSettings->UIManagerClass)
	{
		return;
	}

	m_pUIManager = NewObject<ULxUIManager>(this, GameSettings->UIManagerClass);
	if (m_pUIManager)
	{
		m_pUIManager->InitializeManager(this);
	}
}

void ULxLocalPlayerSubsystem::Deinitialize()
{
	m_pUIManager = nullptr;
	m_pControlledCharacter = nullptr;
	m_pPlayerController = nullptr;
	m_pInputComponentQuote = nullptr;

	Super::Deinitialize();
}

ULxLocalPlayerSubsystem* ULxLocalPlayerSubsystem::GetFromLocalPlayer(const ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<ULxLocalPlayerSubsystem>();
}

void ULxLocalPlayerSubsystem::RegisterInputReceive(FName InInputName,
	TScriptInterface<ILxInputReceiveInterface> InRegisterObj)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote->RegisterInputReceive(InInputName, InRegisterObj);
}

void ULxLocalPlayerSubsystem::UnregisterInputReceive(FName InInputName)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote->UnregisterInputReceive(InInputName);
}

void ULxLocalPlayerSubsystem::SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote)
{
	if (!InUInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote = InUInputComponentQuote;
	if (m_pUIManager)
	{
		m_pUIManager->InitMonitorRegistration();
	}
}

void ULxLocalPlayerSubsystem::SetPlayerControllerQuote(ALxPlayerController* InPlayerController)
{
	m_pPlayerController = InPlayerController;

	if (m_pUIManager)
	{
		m_pUIManager->SetPlayerController(InPlayerController);
	}
}

void ULxLocalPlayerSubsystem::SetControlledCharacter(ALxBaseCharacter* InCharacter)
{
	if (m_pControlledCharacter == InCharacter)
	{
		return;
	}
	m_pControlledCharacter = InCharacter;

	if (m_pUIManager)
	{
		m_pUIManager->SetControlledCharacter(InCharacter);
	}
}
