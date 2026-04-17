#include "LxUIFunctionTypes.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"

bool ULxMainMenuUIFunction::ShouldDisplayCursor() const
{
	return HasAnyVisibleManagedUI();
}

bool ULxCharacterFunctionUIFunction::ShouldDisplayCursor() const
{
	return HasAnyVisibleManagedUI();
}

bool ULxCharacterInteractionUIFunction::ShouldDisplayCursor() const
{
	return HasAnyVisibleManagedUI();
}

bool ULxCharacterHUDUIFunction::ShouldDisplayCursor() const
{
	return HasAnyVisibleManagedUI();
}

bool ULxCharacterPopupUIFunction::ShouldDisplayCursor() const
{
	return HasAnyVisibleManagedUI();
}

bool ULxCharacterPopupUIFunction::ShowItemTooltip(ULxItemLogicBase* InItemLogic, FVector2D InMouseScreenPosition)
{
	ULxItemTooltipWidget* ItemTooltipWidget = GetItemTooltipWidget();
	if (!ItemTooltipWidget || !ItemTooltipWidget->SetDisplayItemLogic(InItemLogic))
	{
		return false;
	}

	ItemTooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemTooltipWidget->SetTooltipScreenPosition(InMouseScreenPosition + FVector2D(100.0f, 100.0f));
	NotifyManagedUIVisibilityChanged(ItemTooltipWidget);
	return true;
}

void ULxCharacterPopupUIFunction::UpdateItemTooltipPosition(FVector2D InMouseScreenPosition)
{
	ULxItemTooltipWidget* ItemTooltipWidget = GetItemTooltipWidget();
	if (!ItemTooltipWidget || !IsManagedUIVisible(ItemTooltipWidget))
	{
		return;
	}

	ItemTooltipWidget->SetTooltipScreenPosition(InMouseScreenPosition + FVector2D(100.0f, 100.0f));
}

void ULxCharacterPopupUIFunction::HideItemTooltip()
{
	ULxItemTooltipWidget* ItemTooltipWidget = GetItemTooltipWidget();
	if (!ItemTooltipWidget)
	{
		return;
	}

	ItemTooltipWidget->SetVisibility(ESlateVisibility::Collapsed);
	NotifyManagedUIVisibilityChanged(ItemTooltipWidget);
}

void ULxCharacterPopupUIFunction::AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID)
{
	Super::AddManagedUIWidget(InChildUIWidget, InInputActionID);

	if (ULxItemTooltipWidget* ItemTooltipWidget = Cast<ULxItemTooltipWidget>(InChildUIWidget))
	{
		m_pItemTooltipWidget = ItemTooltipWidget;
	}
}

void ULxCharacterPopupUIFunction::RemoveManagedUIWidget(ULxUIBaseObject* InChildUIWidget)
{
	if (m_pItemTooltipWidget == InChildUIWidget)
	{
		m_pItemTooltipWidget = nullptr;
	}

	Super::RemoveManagedUIWidget(InChildUIWidget);
}

ULxItemTooltipWidget* ULxCharacterPopupUIFunction::GetItemTooltipWidget() const
{
	if (m_pItemTooltipWidget)
	{
		return m_pItemTooltipWidget;
	}

	for (ULxUIBaseObject* ManagedWidget : ManagedWidgets)
	{
		if (ULxItemTooltipWidget* ItemTooltipWidget = Cast<ULxItemTooltipWidget>(ManagedWidget))
		{
			return ItemTooltipWidget;
		}
	}

	return nullptr;
}
