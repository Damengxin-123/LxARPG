#include "LxTextUI.h"

#include "LxUITextData.h"


void ULxTextUI::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	CreateText(ListItemObject);
}

void ULxTextUI::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void ULxTextUI::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
}

void ULxTextUI::CreateText(UObject* ListItemObject) const
{
	if (ULxUITextData* TextData = Cast<ULxUITextData>(ListItemObject))
	{
		OnShowTextEvent.Broadcast(TextData->DisplayText, TextData->IsDarkColor);
	}
}
