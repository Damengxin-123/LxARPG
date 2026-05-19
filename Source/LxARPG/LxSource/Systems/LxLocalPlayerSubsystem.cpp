#include "LxLocalPlayerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

void ULxLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void ULxLocalPlayerSubsystem::Deinitialize()
{
	if (m_pUIManager)
	{
		m_pUIManager->RemoveFromParent();
	}

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

void ULxLocalPlayerSubsystem::RegisterInputReceive(ELxInputActionID InInputActionID,
	TScriptInterface<ILxInputReceiveInterface> InRegisterObj)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote->RegisterInputReceive(InInputActionID, InRegisterObj);
}

void ULxLocalPlayerSubsystem::UnregisterInputReceive(ELxInputActionID InInputActionID)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote->UnregisterInputReceive(InInputActionID);
}

void ULxLocalPlayerSubsystem::UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote->UnregisterInputReceive(InInputActionID, InRegisterObj);
}

ULxInteractionUIManager* ULxLocalPlayerSubsystem::GetInteractionUIManager() const
{
	return m_pUIManager ? m_pUIManager->GetInteractionUIManager() : nullptr;
}

void ULxLocalPlayerSubsystem::SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote)
{
	if (!InUInputComponentQuote)
	{
		return;
	}

	m_pInputComponentQuote = InUInputComponentQuote;
}

void ULxLocalPlayerSubsystem::SetPlayerControllerQuote(ALxPlayerController* InPlayerController)
{
	m_pPlayerController = InPlayerController;

	if (!m_pPlayerController)
	{
		return;
	}

	const ULxGameSettings* GameSettings = GetDefault<ULxGameSettings>();

	if (!m_pUIManager && GameSettings && GameSettings->UIManagerClass)
	{
		m_pUIManager = CreateWidget<ULxUIManager>(m_pPlayerController, GameSettings->UIManagerClass);
		if (m_pUIManager)
		{
			m_pUIManager->RefreshUI();
			m_pUIManager->AddToPlayerScreen(10);
		}
	}

	if (m_pUIManager)
	{
		m_pUIManager->SetPlayerController(InPlayerController);
		m_pUIManager->SetControlledCharacter(m_pControlledCharacter);
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
