#include "LxUIFunctionBase.h"

#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"

void ULxUIFunctionBase::InitializeFunction(ULxUIManager* InOwnerUIManager)
{
	m_pOwnerUIManager = InOwnerUIManager;
	OnInitialize();
}

void ULxUIFunctionBase::SetPlayerController(ALxPlayerController* InPlayerController)
{
	m_pPlayerController = InPlayerController;

	for (ULxUIBaseObject* Widget : ManagedWidgets)
	{
		if (Widget)
		{
			Widget->UpdatePlayerController(InPlayerController);
		}
	}
}

void ULxUIFunctionBase::SetControlledCharacter(ALxBaseCharacter* InControlledCharacter)
{
	m_pControlledCharacter = InControlledCharacter;

	for (ULxUIBaseObject* Widget : ManagedWidgets)
	{
		if (Widget)
		{
			Widget->UpdateUIComponents(InControlledCharacter);
		}
	}
}

void ULxUIFunctionBase::AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID)
{
	if (!InChildUIWidget)
	{
		return;
	}

	if (!ManagedWidgets.Contains(InChildUIWidget))
	{
		ManagedWidgets.Add(InChildUIWidget);
	}

	TArray<FName> InputActionIDsToRemove;
	for (const TPair<FName, TObjectPtr<ULxUIBaseObject>>& InputPair : InputActionToWidgetMap)
	{
		if (InputPair.Value == InChildUIWidget && InputPair.Key != InInputActionID)
		{
			InputActionIDsToRemove.Add(InputPair.Key);
		}
	}

	for (const FName& InputActionID : InputActionIDsToRemove)
	{
		InputActionToWidgetMap.Remove(InputActionID);
	}

	if (!InInputActionID.IsNone())
	{
		InputActionToWidgetMap.FindOrAdd(InInputActionID) = InChildUIWidget;
	}

	InChildUIWidget->UpdatePlayerController(m_pPlayerController);
	InChildUIWidget->UpdateUIComponents(m_pControlledCharacter);
	SyncVisibleManagedWidget(InChildUIWidget);
}

void ULxUIFunctionBase::RemoveManagedUIWidget(ULxUIBaseObject* InChildUIWidget)
{
	if (!InChildUIWidget)
	{
		return;
	}

	ManagedWidgets.Remove(InChildUIWidget);
	VisibleManagedWidgets.Remove(InChildUIWidget);

	TArray<FName> InputActionIDsToRemove;
	for (const TPair<FName, TObjectPtr<ULxUIBaseObject>>& InputPair : InputActionToWidgetMap)
	{
		if (InputPair.Value == InChildUIWidget)
		{
			InputActionIDsToRemove.Add(InputPair.Key);
		}
	}

	for (const FName& InputActionID : InputActionIDsToRemove)
	{
		InputActionToWidgetMap.Remove(InputActionID);
	}
}

void ULxUIFunctionBase::NotifyManagedUIVisibilityChanged(ULxUIBaseObject* InChildUIWidget)
{
	SyncVisibleManagedWidget(InChildUIWidget);
}

void ULxUIFunctionBase::SetManagedUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible)
{
	if (!InChildUIWidget)
	{
		return;
	}

	InChildUIWidget->SetVisibility(bInVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SyncVisibleManagedWidget(InChildUIWidget);
}

void ULxUIFunctionBase::ToggleManagedUI(ULxUIBaseObject* InChildUIWidget)
{
	if (!InChildUIWidget)
	{
		return;
	}

	SetManagedUIVisible(InChildUIWidget, !IsManagedUIVisible(InChildUIWidget));
}

void ULxUIFunctionBase::HandlePlayerInputAction(FName InInputActionID, const FLxInputValue& InValue)
{
	ULxUIBaseObject* TargetWidget = InputActionToWidgetMap.FindRef(InInputActionID);
	if (!TargetWidget)
	{
		return;
	}

	if (TargetWidget->HandleInputEvent(InInputActionID, InValue))
	{
		SyncVisibleManagedWidget(TargetWidget);
		return;
	}

	if (!InValue.m_blValue)
	{
		return;
	}

	ToggleManagedUI(TargetWidget);
}

void ULxUIFunctionBase::RefreshManagedUIState()
{
	for (ULxUIBaseObject* Widget : ManagedWidgets)
	{
		if (!Widget)
		{
			continue;
		}

		Widget->UpdatePlayerController(m_pPlayerController);
		Widget->UpdateUIComponents(m_pControlledCharacter);
		SyncVisibleManagedWidget(Widget);
	}
}

void ULxUIFunctionBase::OnInitialize()
{
}

bool ULxUIFunctionBase::HasAnyVisibleManagedUI() const
{
	return VisibleManagedWidgets.Num() > 0;
}

bool ULxUIFunctionBase::IsManagedUIVisible(const ULxUIBaseObject* InChildUIWidget) const
{
	return InChildUIWidget && InChildUIWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& InChildUIWidget->GetVisibility() != ESlateVisibility::Hidden;
}

void ULxUIFunctionBase::SyncVisibleManagedWidget(ULxUIBaseObject* InChildUIWidget)
{
	if (!InChildUIWidget)
	{
		return;
	}

	VisibleManagedWidgets.Remove(InChildUIWidget);
	if (IsManagedUIVisible(InChildUIWidget))
	{
		VisibleManagedWidgets.Add(InChildUIWidget);
	}
}
