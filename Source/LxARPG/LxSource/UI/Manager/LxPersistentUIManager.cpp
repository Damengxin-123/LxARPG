#include "LxPersistentUIManager.h"

#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"

void ULxPersistentUIManager::RegisterPersistentWidget(ULxUIBaseObject* InWidget, bool bInUpdateWithCharacterData)
{
	if (!InWidget)
	{
		return;
	}

	PersistentWidgets.AddUnique(InWidget);

	if (bInUpdateWithCharacterData)
	{
		DataDrivenWidgets.Add(InWidget);
		RefreshWidgetData(InWidget);
	}
	else
	{
		DataDrivenWidgets.Remove(InWidget);
	}
}

void ULxPersistentUIManager::UnregisterPersistentWidget(ULxUIBaseObject* InWidget)
{
	PersistentWidgets.Remove(InWidget);
	DataDrivenWidgets.Remove(InWidget);
}

void ULxPersistentUIManager::RefreshManagedUI()
{
	for (ULxUIBaseObject* Widget : PersistentWidgets)
	{
		if (DataDrivenWidgets.Contains(Widget))
		{
			RefreshWidgetData(Widget);
		}
	}
}

bool ULxPersistentUIManager::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return PersistentWidgets.Contains(InWidget);
}
