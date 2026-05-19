#include "LxTooltipUIManager.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

void ULxTooltipUIManager::SetItemTooltipWidget(ULxItemTooltipWidget* InItemTooltipWidget, bool bInShowCursorWhenVisible)
{
	ItemTooltipWidget = InItemTooltipWidget;
	bShowCursorWhenVisible = bInShowCursorWhenVisible;
	if (ItemTooltipWidget)
	{
		SetWidgetVisibility(ItemTooltipWidget, false);
	}
}

bool ULxTooltipUIManager::ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition)
{
	return ShowItemTooltipWithValue(InItem, 0, false, InMouseScreenPosition);
}

bool ULxTooltipUIManager::ShowItemTooltipWithValue(ULxItemBase* InItem, int32 InItemValue, bool bInShowItemValue, FVector2D InMouseScreenPosition)
{
	if (!ItemTooltipWidget || !ItemTooltipWidget->SetDisplayItemLogicWithValue(InItem, InItemValue, bInShowItemValue))
	{
		return false;
	}

	ItemTooltipWidget->SetVisibility(ESlateVisibility::Visible);
	UpdateTooltipPosition(ItemTooltipWidget, InMouseScreenPosition);
	return true;
}

void ULxTooltipUIManager::UpdateItemTooltipPosition(FVector2D InMouseScreenPosition)
{
	if (!HasVisibleTooltip())
	{
		return;
	}

	UpdateTooltipPosition(ItemTooltipWidget, InMouseScreenPosition);
}

void ULxTooltipUIManager::HideItemTooltip()
{
	SetWidgetVisibility(ItemTooltipWidget, false);
}

bool ULxTooltipUIManager::HasVisibleTooltip() const
{
	return IsWidgetVisible(ItemTooltipWidget);
}

bool ULxTooltipUIManager::ShouldShowCursorForTooltip() const
{
	return bShowCursorWhenVisible && HasVisibleTooltip();
}

bool ULxTooltipUIManager::ContainsWidget(const ULxUIBaseObject* InWidget) const
{
	return ItemTooltipWidget == InWidget;
}

void ULxTooltipUIManager::UpdateTooltipPosition(ULxUIBaseObject* InWidget, FVector2D InAnchorScreenPosition)
{
	if (!InWidget || !OwningUIManager)
	{
		return;
	}

	const FVector2D WidgetSize = InWidget->GetDesiredSize();
	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	ViewportSize = DPIScale > 0.0f ? ViewportSize / DPIScale : ViewportSize;

	FVector2D FinalPos = InAnchorScreenPosition + FVector2D(12.0f, 12.0f);
	if (InAnchorScreenPosition.X + WidgetSize.X + 12.0f > ViewportSize.X)
	{
		FinalPos.X = InAnchorScreenPosition.X - WidgetSize.X - 12.0f;
	}

	if (InAnchorScreenPosition.Y + WidgetSize.Y + 12.0f > ViewportSize.Y)
	{
		FinalPos.Y = InAnchorScreenPosition.Y - WidgetSize.Y - 12.0f;
	}

	OwningUIManager->UpdateManagedUIPosition(InWidget, FinalPos);
}
