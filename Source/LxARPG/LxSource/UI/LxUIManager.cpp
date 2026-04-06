#include "LxUIManager.h"

#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/Backpack/LxBackpackWidget.h"
#include "LxARPG/LxSource/UI/CharacterAttributeUI/LxAttributeWidget.h"

void ULxUIManager::InitializeManager(ULxLocalPlayerSubsystem* InLocalPlayerSubsystem)
{
	m_pLocalPlayerSubsystem = InLocalPlayerSubsystem;
	CreateUIWidgets();
	InitMonitorRegistration();
	RefreshUI();
}

void ULxUIManager::SetPlayerController(ALxPlayerController* InPlayerController)
{
	m_pPlayerController = InPlayerController;
	CreateUIWidgets();
	ApplyControllerToWidgets();
	RefreshUI();
}

void ULxUIManager::SetControlledCharacter(ALxBaseCharacter* InCharacter)
{
	m_pControlledCharacter = InCharacter;
	ApplyCharacterToWidgets();
}

void ULxUIManager::RefreshUI()
{
	CreateUIWidgets();
	ApplyControllerToWidgets();
	ApplyCharacterToWidgets();
}

void ULxUIManager::HandleInputValue(FName InName, FLxInputValue InValue)
{
	if (!InValue.m_blValue)
	{
		return;
	}

	if (InName == OpenBackpackInputActionID)
	{
		ToggleBackpackUI();
	}
	else if (InName == OpenCharacterInformationInputActionID)
	{
		ToggleAttributeUI();
	}
}

void ULxUIManager::InitMonitorRegistration()
{
	if (!m_pLocalPlayerSubsystem || m_bInputRegistered || !m_pLocalPlayerSubsystem->HasInputComponentQuote())
	{
		return;
	}

	TScriptInterface<ILxInputReceiveInterface> InputReceive;
	InputReceive.SetObject(this);
	InputReceive.SetInterface(Cast<ILxInputReceiveInterface>(this));

	m_pLocalPlayerSubsystem->RegisterInputReceive(OpenBackpackInputActionID, InputReceive);
	m_pLocalPlayerSubsystem->RegisterInputReceive(OpenCharacterInformationInputActionID, InputReceive);
	m_bInputRegistered = true;
}

void ULxUIManager::ToggleBackpackUI()
{
	CreateUIWidgets();
	if (!m_pBackpackWidget)
	{
		return;
	}

	SetWidgetVisible(m_pBackpackWidget, !IsWidgetVisible(m_pBackpackWidget));
	UpdateCursorState();
}

void ULxUIManager::ToggleAttributeUI()
{
	CreateUIWidgets();
	if (!m_pAttributeWidget)
	{
		return;
	}

	SetWidgetVisible(m_pAttributeWidget, !IsWidgetVisible(m_pAttributeWidget));
	UpdateCursorState();
}

void ULxUIManager::CreateUIWidgets()
{
	if (!m_pPlayerController)
	{
		return;
	}

	if (!m_pBackpackWidget && BackpackWidgetClass)
	{
		m_pBackpackWidget = CreateWidget<ULxBackpackWidget>(m_pPlayerController, BackpackWidgetClass);
		if (m_pBackpackWidget)
		{
			m_pBackpackWidget->AddToViewport();
			SetWidgetVisible(m_pBackpackWidget, false);
		}
	}

	if (!m_pAttributeWidget && AttributeWidgetClass)
	{
		m_pAttributeWidget = CreateWidget<ULxAttributeWidget>(m_pPlayerController, AttributeWidgetClass);
		if (m_pAttributeWidget)
		{
			m_pAttributeWidget->AddToViewport();
			SetWidgetVisible(m_pAttributeWidget, false);
		}
	}
}

void ULxUIManager::ApplyCharacterToWidgets()
{
	if (m_pBackpackWidget)
	{
		m_pBackpackWidget->UpdateUIComponents(m_pControlledCharacter);
	}

	if (m_pAttributeWidget)
	{
		m_pAttributeWidget->UpdateUIComponents(m_pControlledCharacter);
	}
}

void ULxUIManager::ApplyControllerToWidgets()
{
	if (m_pBackpackWidget)
	{
		m_pBackpackWidget->UpdatePlayerController(m_pPlayerController);
	}

	if (m_pAttributeWidget)
	{
		m_pAttributeWidget->UpdatePlayerController(m_pPlayerController);
	}
}

void ULxUIManager::SetWidgetVisible(UUserWidget* InWidget, bool bInVisible)
{
	if (!InWidget)
	{
		return;
	}

	InWidget->SetVisibility(bInVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

bool ULxUIManager::IsWidgetVisible(const UUserWidget* InWidget) const
{
	return InWidget && InWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& InWidget->GetVisibility() != ESlateVisibility::Hidden;
}

void ULxUIManager::UpdateCursorState() const
{
	if (!m_pPlayerController)
	{
		return;
	}

	if (IsWidgetVisible(m_pBackpackWidget) || IsWidgetVisible(m_pAttributeWidget))
	{
		m_pPlayerController->ShowCursorFun();
	}
	else
	{
		m_pPlayerController->HideCursorFun();
	}
}
