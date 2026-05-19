#include "LxTogglePanelUIManager.h"

#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

void ULxTogglePanelUIManager::RegisterPanelWidget(ULxUIBaseObject* InWidget, ELxInputActionID InInputActionID,
	bool bInShowCursorWhenVisible, bool bInCloseOtherPanelsWhenOpened, bool bInUpdateWithCharacterData)
{
	if (!InWidget)
	{
		return;
	}

	FLxTogglePanelWidgetData* PanelData = FindPanelDataByWidget(InWidget);
	if (!PanelData)
	{
		PanelData = &PanelWidgets.AddDefaulted_GetRef();
		PanelData->UIWidget = InWidget;
	}

	UpdateInputRegistration(*PanelData, InInputActionID);
	PanelData->bShowCursorWhenVisible = bInShowCursorWhenVisible;
	PanelData->bCloseOtherPanelsWhenOpened = bInCloseOtherPanelsWhenOpened;
	PanelData->bUpdateWithCharacterData = bInUpdateWithCharacterData;

	if (bInUpdateWithCharacterData)
	{
		RefreshWidgetData(InWidget);
	}
}

bool ULxTogglePanelUIManager::SetPanelVisible(ULxUIBaseObject* InWidget, bool bInVisible)
{
	const FLxTogglePanelWidgetData* PanelData = FindPanelDataByWidget(InWidget);
	if (!PanelData)
	{
		return false;
	}

	if (bInVisible && PanelData->bCloseOtherPanelsWhenOpened)
	{
		CloseOtherPanels(InWidget);
	}

	SetWidgetVisibility(InWidget, bInVisible);
	return true;
}

bool ULxTogglePanelUIManager::TogglePanelWidget(ULxUIBaseObject* InWidget)
{
	if (!InWidget || !ContainsWidget(InWidget))
	{
		return false;
	}

	return SetPanelVisible(InWidget, !IsWidgetVisible(InWidget));
}

bool ULxTogglePanelUIManager::HandleInputValue(ELxInputActionID InInputActionID, const FLxInputValue& InValue)
{
	ULxUIBaseObject* TargetWidget = InputActionToWidget.FindRef(InInputActionID);
	if (!TargetWidget)
	{
		return false;
	}

	if (TargetWidget->HandleInputEvent(InInputActionID, InValue))
	{
		return true;
	}

	if (InValue.m_blValue)
	{
		TogglePanelWidget(TargetWidget);
		return true;
	}

	return false;
}

bool ULxTogglePanelUIManager::HasVisibleCursorPanel() const
{
	for (const FLxTogglePanelWidgetData& PanelData : PanelWidgets)
	{
		if (PanelData.bShowCursorWhenVisible && IsWidgetVisible(PanelData.UIWidget))
		{
			return true;
		}
	}

	return false;
}

void ULxTogglePanelUIManager::RefreshManagedUI()
{
	for (const FLxTogglePanelWidgetData& PanelData : PanelWidgets)
	{
		if (PanelData.bUpdateWithCharacterData)
		{
			RefreshWidgetData(PanelData.UIWidget);
		}
	}
}

bool ULxTogglePanelUIManager::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return FindPanelDataByWidget(InWidget) != nullptr;
}

FLxTogglePanelWidgetData* ULxTogglePanelUIManager::FindPanelDataByWidget(const ULxUIBaseObject* InWidget)
{
	return PanelWidgets.FindByPredicate(
		[InWidget](const FLxTogglePanelWidgetData& PanelData)
		{
			return PanelData.UIWidget == InWidget;
		});
}

const FLxTogglePanelWidgetData* ULxTogglePanelUIManager::FindPanelDataByWidget(const ULxUIBaseObject* InWidget) const
{
	return PanelWidgets.FindByPredicate(
		[InWidget](const FLxTogglePanelWidgetData& PanelData)
		{
			return PanelData.UIWidget == InWidget;
		});
}

void ULxTogglePanelUIManager::CloseOtherPanels(const ULxUIBaseObject* InKeepWidget)
{
	for (const FLxTogglePanelWidgetData& PanelData : PanelWidgets)
	{
		if (PanelData.UIWidget != InKeepWidget)
		{
			SetWidgetVisibility(PanelData.UIWidget, false);
		}
	}
}

void ULxTogglePanelUIManager::UpdateInputRegistration(FLxTogglePanelWidgetData& InPanelData, ELxInputActionID InNewInputActionID)
{
	if (OwningUIManager && InPanelData.InputActionID != ELxInputActionID::None && InPanelData.InputActionID != InNewInputActionID)
	{
		if (InputActionToWidget.FindRef(InPanelData.InputActionID) == InPanelData.UIWidget)
		{
			InputActionToWidget.Remove(InPanelData.InputActionID);
			OwningUIManager->UnregisterInputActionReceive(InPanelData.InputActionID);
		}
	}

	InPanelData.InputActionID = InNewInputActionID;

	if (OwningUIManager && InNewInputActionID != ELxInputActionID::None)
	{
		InputActionToWidget.FindOrAdd(InNewInputActionID) = InPanelData.UIWidget;
		OwningUIManager->RegisterInputActionReceive(InNewInputActionID);
	}
}
