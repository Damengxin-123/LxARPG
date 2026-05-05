#include "LxUIFunctionTypes.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

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
	return false;
}

void ULxCharacterHUDUIFunction::SetControlledCharacter(ALxBaseCharacter* InControlledCharacter)
{
	Super::SetControlledCharacter(InControlledCharacter);

	const bool bShouldBeVisible = InControlledCharacter != nullptr;
	for (ULxUIBaseObject* ManagedWidget : ManagedWidgets)
	{
		if (!ManagedWidget)
		{
			continue;
		}

		ManagedWidget->SetVisibility(bShouldBeVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		SyncVisibleManagedWidget(ManagedWidget);
	}
}

void ULxCharacterHUDUIFunction::AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID)
{
	Super::AddManagedUIWidget(InChildUIWidget, InInputActionID);

	if (!InChildUIWidget)
	{
		return;
	}

	InChildUIWidget->SetVisibility(GetCurrentControlledCharacter() ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	SyncVisibleManagedWidget(InChildUIWidget);
}

void ULxCharacterHUDUIFunction::HandlePlayerInputAction(FName InInputActionID, const FLxInputValue& InValue)
{
	if (ULxItemGridWidget* TargetGridWidget = ShortcutInputToGridMap.FindRef(InInputActionID))
	{
		if (InValue.m_blValue)
		{
			TargetGridWidget->UseItem();
		}
		return;
	}

	Super::HandlePlayerInputAction(InInputActionID, InValue);
}

bool ULxCharacterHUDUIFunction::BindShortcutInputAction(ULxItemGridWidget* InItemGridWidget, FName InInputActionID)
{
	if (!InItemGridWidget || InInputActionID.IsNone() || !m_pOwnerUIManager)
	{
		return false;
	}

	TArray<FName> InputActionsToRemove;
	for (const TPair<FName, TObjectPtr<ULxItemGridWidget>>& Pair : ShortcutInputToGridMap)
	{
		if (Pair.Value == InItemGridWidget && Pair.Key != InInputActionID)
		{
			InputActionsToRemove.Add(Pair.Key);
		}
	}

	for (const FName& InputActionID : InputActionsToRemove)
	{
		ShortcutInputToGridMap.Remove(InputActionID);
	}

	ShortcutInputToGridMap.FindOrAdd(InInputActionID) = InItemGridWidget;
	m_pOwnerUIManager->RegisterUIFunctionInputAction(ELxUIFunctionType::CharacterHUD, InInputActionID);
	return true;
}

bool ULxCharacterPopupUIFunction::ShouldDisplayCursor() const
{
	return HasAnyVisibleManagedUI();
}

bool ULxCharacterPopupUIFunction::ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition)
{
	ULxItemTooltipWidget* ItemTooltipWidget = GetItemTooltipWidget();
	if (!ItemTooltipWidget || !ItemTooltipWidget->SetDisplayItemLogic(InItem))
	{
		return false;
	}

	ItemTooltipWidget->SetVisibility(ESlateVisibility::Visible);
	RequestManagedUIPosition(ItemTooltipWidget, InMouseScreenPosition, FVector2D(100.0f, 100.0f), true);
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

	RequestManagedUIPosition(ItemTooltipWidget, InMouseScreenPosition, FVector2D(100.0f, 100.0f), true);
}

void ULxCharacterPopupUIFunction::HideItemTooltip()
{
	ULxItemTooltipWidget* ItemTooltipWidget = GetItemTooltipWidget();
	if (!ItemTooltipWidget)
	{
		return;
	}

	ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
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
