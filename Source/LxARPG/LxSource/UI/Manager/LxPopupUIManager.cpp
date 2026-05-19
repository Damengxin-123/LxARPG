#include "LxPopupUIManager.h"

#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"

void ULxPopupUIManager::RegisterPopupWidget(ULxUIBaseObject* InWidget, bool bInHideOnRegister)
{
	if (!InWidget)
	{
		return;
	}

	PopupWidgets.AddUnique(InWidget);
	RefreshWidgetData(InWidget);

	if (bInHideOnRegister)
	{
		SetWidgetVisibility(InWidget, false);
	}
}

bool ULxPopupUIManager::ShowPopup(ULxUIBaseObject* InWidget)
{
	if (!ContainsWidget(InWidget))
	{
		return false;
	}

	SetWidgetVisibility(InWidget, true);
	return true;
}

bool ULxPopupUIManager::HidePopup(ULxUIBaseObject* InWidget)
{
	if (!ContainsWidget(InWidget))
	{
		return false;
	}

	SetWidgetVisibility(InWidget, false);
	return true;
}

void ULxPopupUIManager::HideAllPopups()
{
	for (ULxUIBaseObject* PopupWidget : PopupWidgets)
	{
		SetWidgetVisibility(PopupWidget, false);
	}
}

bool ULxPopupUIManager::HasVisiblePopup() const
{
	for (ULxUIBaseObject* PopupWidget : PopupWidgets)
	{
		if (IsWidgetVisible(PopupWidget))
		{
			return true;
		}
	}

	return false;
}

void ULxPopupUIManager::RefreshManagedUI()
{
	for (ULxUIBaseObject* PopupWidget : PopupWidgets)
	{
		RefreshWidgetData(PopupWidget);
	}
}

bool ULxPopupUIManager::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return PopupWidgets.Contains(InWidget);
}
