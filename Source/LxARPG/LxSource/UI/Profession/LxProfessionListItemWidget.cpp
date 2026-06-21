#include "LxProfessionListItemWidget.h"

#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "LxProfessionUIData.h"

void ULxProfessionListItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	ProfessionListItemData = Cast<ULxProfessionListItemUIData>(ListItemObject);
	OnProfessionListItemUpdated(ProfessionListItemData);
}

void ULxProfessionListItemWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	OnProfessionListItemSelectedStateUpdated(ProfessionListItemData, bIsSelected);
}

FReply ULxProfessionListItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ProfessionListItemData != nullptr)
	{
		ProfessionListItemData->RequestSelectProfession();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
