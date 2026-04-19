#include "LxUIManager.h"

#include "Components/CanvasPanelSlot.h"
#include "LxUIFunctionBase.h"
#include "LxUIFunctionTypes.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"

void ULxUIManager::InitializeManager(ULxLocalPlayerSubsystem* InLocalPlayerSubsystem)
{
	m_pLocalPlayerSubsystem = InLocalPlayerSubsystem;
	InitializeFunctionObjects();
	InitMonitorRegistration();
	RefreshUI();
}

void ULxUIManager::SetPlayerController(ALxPlayerController* InPlayerController)
{
	m_pPlayerController = InPlayerController;
	UpdatePlayerController(InPlayerController);

	for (TPair<ELxUIFunctionType, TObjectPtr<ULxUIFunctionBase>>& FunctionPair : m_mapUITypeToFunction)
	{
		if (FunctionPair.Value)
		{
			FunctionPair.Value->SetPlayerController(InPlayerController);
		}
	}

	UpdateCursorState();
}

void ULxUIManager::SetControlledCharacter(ALxBaseCharacter* InCharacter)
{
	m_pControlledCharacter = InCharacter;
	UpdateUIComponents(InCharacter);

	for (TPair<ELxUIFunctionType, TObjectPtr<ULxUIFunctionBase>>& FunctionPair : m_mapUITypeToFunction)
	{
		if (FunctionPair.Value)
		{
			FunctionPair.Value->SetControlledCharacter(InCharacter);
		}
	}

	UpdateCursorState();
}

void ULxUIManager::RefreshUI()
{
	for (TPair<ELxUIFunctionType, TObjectPtr<ULxUIFunctionBase>>& FunctionPair : m_mapUITypeToFunction)
	{
		if (!FunctionPair.Value)
		{
			continue;
		}

		FunctionPair.Value->SetPlayerController(m_pPlayerController);
		FunctionPair.Value->SetControlledCharacter(m_pControlledCharacter);
		FunctionPair.Value->RefreshManagedUIState();
	}

	UpdateCursorState();
}

void ULxUIManager::RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID, ELxUIFunctionType InUIType)
{
	if (!InChildUIWidget)
	{
		return;
	}

	FLxManagedUIWidgetData* ExistData = FindManagedUIDataByWidget(InChildUIWidget);
	ELxUIFunctionType PreviousUIType = InUIType;
	FName PreviousInputActionID = NAME_None;

	if (!ExistData)
	{
		FLxManagedUIWidgetData& NewData = RegisteredChildWidgets.AddDefaulted_GetRef();
		NewData.UIWidget = InChildUIWidget;
		ExistData = &NewData;
	}
	else
	{
		PreviousUIType = ExistData->UIType;
		PreviousInputActionID = ExistData->InputActionID;
	}

	if (ULxUIFunctionBase* PreviousFunction = GetOrCreateUIFunction(PreviousUIType))
	{
		if (PreviousUIType != InUIType || PreviousInputActionID != InInputActionID)
		{
			PreviousFunction->RemoveManagedUIWidget(InChildUIWidget);
		}
	}

	ExistData->InputActionID = InInputActionID;
	ExistData->UIType = InUIType;

	ULxUIFunctionBase* TargetFunction = GetOrCreateUIFunction(InUIType);
	if (!TargetFunction)
	{
		return;
	}

	TargetFunction->AddManagedUIWidget(InChildUIWidget, InInputActionID);

	if (!PreviousInputActionID.IsNone() && PreviousInputActionID != InInputActionID)
	{
		m_mapInputActionToFunction.Remove(PreviousInputActionID);
	}

	if (!InInputActionID.IsNone())
	{
		m_mapInputActionToFunction.FindOrAdd(InInputActionID) = TargetFunction;
		RegisterInputAction(InInputActionID);
	}

	UpdateCursorState();
}

void ULxUIManager::NotifyChildUIVisibilityChanged(ULxUIBaseObject* InChildUIWidget)
{
	FLxManagedUIWidgetData* WidgetData = FindManagedUIDataByWidget(InChildUIWidget);
	if (!WidgetData)
	{
		return;
	}

	if (ULxUIFunctionBase* UIFunction = GetOrCreateUIFunction(WidgetData->UIType))
	{
		UIFunction->NotifyManagedUIVisibilityChanged(InChildUIWidget);
	}

	UpdateCursorState();
}

void ULxUIManager::SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible)
{
	FLxManagedUIWidgetData* WidgetData = FindManagedUIDataByWidget(InChildUIWidget);
	if (!WidgetData)
	{
		return;
	}

	if (ULxUIFunctionBase* UIFunction = GetOrCreateUIFunction(WidgetData->UIType))
	{
		UIFunction->SetManagedUIVisible(InChildUIWidget, bInVisible);
	}

	UpdateCursorState();
}

void ULxUIManager::ToggleChildUI(ULxUIBaseObject* InChildUIWidget)
{
	FLxManagedUIWidgetData* WidgetData = FindManagedUIDataByWidget(InChildUIWidget);
	if (!WidgetData)
	{
		return;
	}

	if (ULxUIFunctionBase* UIFunction = GetOrCreateUIFunction(WidgetData->UIType))
	{
		UIFunction->ToggleManagedUI(InChildUIWidget);
	}

	UpdateCursorState();
}

void ULxUIManager::UpdateManagedUIPosition_Implementation(ULxUIBaseObject* InChildUIWidget, FVector2D InScreenPosition)
{
	if (!InChildUIWidget)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(InChildUIWidget->Slot))
	{
		CanvasPanelSlot->SetPosition(InScreenPosition);
	}
}

ULxCharacterHUDUIFunction* ULxUIManager::GetCharacterHUDUIFunction() const
{
	return Cast<ULxCharacterHUDUIFunction>(m_mapUITypeToFunction.FindRef(ELxUIFunctionType::CharacterHUD));
}

ULxCharacterPopupUIFunction* ULxUIManager::GetCharacterPopupUIFunction() const
{
	return Cast<ULxCharacterPopupUIFunction>(m_mapUITypeToFunction.FindRef(ELxUIFunctionType::CHaracterPopup));
}

void ULxUIManager::RegisterUIFunctionInputAction(ELxUIFunctionType InUIType, FName InInputActionID)
{
	if (InInputActionID.IsNone())
	{
		return;
	}

	if (ULxUIFunctionBase* TargetFunction = GetOrCreateUIFunction(InUIType))
	{
		m_mapInputActionToFunction.FindOrAdd(InInputActionID) = TargetFunction;
		RegisterInputAction(InInputActionID);
	}
}

void ULxUIManager::HandleInputValue(FName InName, FLxInputValue InValue)
{
	ULxUIFunctionBase* UIFunction = m_mapInputActionToFunction.FindRef(InName);
	if (!UIFunction)
	{
		return;
	}

	UIFunction->HandlePlayerInputAction(InName, InValue);
	UpdateCursorState();
}

void ULxUIManager::InitMonitorRegistration()
{
	if (!m_pLocalPlayerSubsystem || !m_pLocalPlayerSubsystem->HasInputComponentQuote())
	{
		return;
	}

	for (const FLxManagedUIWidgetData& WidgetData : RegisteredChildWidgets)
	{
		RegisterInputAction(WidgetData.InputActionID);
	}
}

FLxManagedUIWidgetData* ULxUIManager::FindManagedUIDataByWidget(ULxUIBaseObject* InChildUIWidget)
{
	return RegisteredChildWidgets.FindByPredicate(
		[InChildUIWidget](const FLxManagedUIWidgetData& WidgetData)
		{
			return WidgetData.UIWidget == InChildUIWidget;
		});
}

ULxUIFunctionBase* ULxUIManager::GetOrCreateUIFunction(ELxUIFunctionType InUIType)
{
	if (ULxUIFunctionBase* ExistFunction = m_mapUITypeToFunction.FindRef(InUIType))
	{
		return ExistFunction;
	}

	ULxUIFunctionBase* NewFunction = nullptr;
	switch (InUIType)
	{
	case ELxUIFunctionType::MainMenu:
		NewFunction = NewObject<ULxMainMenuUIFunction>(this);
		break;
	case ELxUIFunctionType::CharacterFunction:
		NewFunction = NewObject<ULxCharacterFunctionUIFunction>(this);
		break;
	case ELxUIFunctionType::CharacterInteraction:
		NewFunction = NewObject<ULxCharacterInteractionUIFunction>(this);
		break;
	case ELxUIFunctionType::CharacterHUD:
		NewFunction = NewObject<ULxCharacterHUDUIFunction>(this);
		break;
	case ELxUIFunctionType::CHaracterPopup:
		NewFunction = NewObject<ULxCharacterPopupUIFunction>(this);
		break;
	default:
		break;
	}

	if (!NewFunction)
	{
		return nullptr;
	}

	NewFunction->InitializeFunction(this);
	NewFunction->SetPlayerController(m_pPlayerController);
	NewFunction->SetControlledCharacter(m_pControlledCharacter);
	m_mapUITypeToFunction.Add(InUIType, NewFunction);
	return NewFunction;
}

void ULxUIManager::InitializeFunctionObjects()
{
	GetOrCreateUIFunction(ELxUIFunctionType::MainMenu);
	GetOrCreateUIFunction(ELxUIFunctionType::CharacterFunction);
	GetOrCreateUIFunction(ELxUIFunctionType::CharacterInteraction);
	GetOrCreateUIFunction(ELxUIFunctionType::CharacterHUD);
	GetOrCreateUIFunction(ELxUIFunctionType::CHaracterPopup);
}

void ULxUIManager::RegisterInputAction(FName InInputActionID)
{
	if (InInputActionID.IsNone() || !m_pLocalPlayerSubsystem || !m_pLocalPlayerSubsystem->HasInputComponentQuote()
		|| RegisteredInputActionIDs.Contains(InInputActionID))
	{
		return;
	}

	TScriptInterface<ILxInputReceiveInterface> InputReceive;
	InputReceive.SetObject(this);
	InputReceive.SetInterface(Cast<ILxInputReceiveInterface>(this));
	m_pLocalPlayerSubsystem->RegisterInputReceive(InInputActionID, InputReceive);
	RegisteredInputActionIDs.Add(InInputActionID);
}

void ULxUIManager::UpdateCursorState() const
{
	if (!m_pPlayerController)
	{
		return;
	}

	for (const TPair<ELxUIFunctionType, TObjectPtr<ULxUIFunctionBase>>& FunctionPair : m_mapUITypeToFunction)
	{
		if (FunctionPair.Value && FunctionPair.Value->ShouldDisplayCursor())
		{
			m_pPlayerController->ShowCursorFun();
			return;
		}
	}

	m_pPlayerController->HideCursorFun();
}
