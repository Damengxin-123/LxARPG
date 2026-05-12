#include "LxUIManager.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"

void ULxUIManager::SetPlayerController(ALxPlayerController* InPlayerController)
{
	m_pPlayerController = InPlayerController;
	UpdateCursorState();
}

void ULxUIManager::SetControlledCharacter(ALxBaseCharacter* InCharacter)
{
	m_pCharacterDataTransferComponent = InCharacter ? InCharacter->GetCharacterDataTransferComponent() : nullptr;
	UpdateUIComponents(m_pCharacterDataTransferComponent);
	RefreshUI();
}

void ULxUIManager::RefreshUI()
{
	for (const FLxManagedUIWidgetData& WidgetData : RegisteredChildWidgets)
	{
		if (WidgetData.UIWidget)
		{
			WidgetData.UIWidget->UpdateUIComponents(m_pCharacterDataTransferComponent);
		}
	}

	UpdateCursorState();
}

void ULxUIManager::RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible)
{
	if (!InChildUIWidget)
	{
		return;
	}

	FLxManagedUIWidgetData* ExistData = FindManagedUIDataByWidget(InChildUIWidget);
	ELxInputActionID PreviousInputActionID = ELxInputActionID::None;

	if (!ExistData)
	{
		FLxManagedUIWidgetData& NewData = RegisteredChildWidgets.AddDefaulted_GetRef();
		NewData.UIWidget = InChildUIWidget;
		ExistData = &NewData;
	}
	else
	{
		PreviousInputActionID = ExistData->InputActionID;
	}

	if (PreviousInputActionID != ELxInputActionID::None && PreviousInputActionID != InInputActionID)
	{
		if (m_mapInputActionToWidget.FindRef(PreviousInputActionID) == InChildUIWidget)
		{
			m_mapInputActionToWidget.Remove(PreviousInputActionID);
		}
	}

	ExistData->InputActionID = InInputActionID;
	ExistData->bShowCursorWhenVisible = bInShowCursorWhenVisible;
	InChildUIWidget->UpdateUIComponents(m_pCharacterDataTransferComponent);

	if (ULxItemTooltipWidget* ItemTooltipWidget = Cast<ULxItemTooltipWidget>(InChildUIWidget))
	{
		m_pItemTooltipWidget = ItemTooltipWidget;
	}

	if (InInputActionID != ELxInputActionID::None)
	{
		RegisterInputActionReceive(InInputActionID);
		m_mapInputActionToWidget.FindOrAdd(InInputActionID) = InChildUIWidget;
	}

	UpdateCursorState();
}

void ULxUIManager::SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible)
{
	if (!FindManagedUIDataByWidget(InChildUIWidget))
	{
		return;
	}

	InChildUIWidget->SetVisibility(bInVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	UpdateCursorState();
}

void ULxUIManager::ToggleChildUI(ULxUIBaseObject* InChildUIWidget)
{
	if (!InChildUIWidget)
	{
		return;
	}

	SetChildUIVisible(InChildUIWidget, !IsManagedUIVisible(InChildUIWidget));
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

bool ULxUIManager::ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition)
{
	if (!m_pItemTooltipWidget || !m_pItemTooltipWidget->SetDisplayItemLogic(InItem))
	{
		return false;
	}

	m_pItemTooltipWidget->SetVisibility(ESlateVisibility::Visible);
	UpdateTooltipPosition(m_pItemTooltipWidget, InMouseScreenPosition);
	UpdateCursorState();
	return true;
}

void ULxUIManager::UpdateItemTooltipPosition(FVector2D InMouseScreenPosition)
{
	if (!m_pItemTooltipWidget || !IsManagedUIVisible(m_pItemTooltipWidget))
	{
		return;
	}

	UpdateTooltipPosition(m_pItemTooltipWidget, InMouseScreenPosition);
}

void ULxUIManager::HideItemTooltip()
{
	if (!m_pItemTooltipWidget)
	{
		return;
	}

	m_pItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
	UpdateCursorState();
}

void ULxUIManager::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	ULxUIBaseObject* TargetWidget = m_mapInputActionToWidget.FindRef(InInputActionID);
	if (!TargetWidget)
	{
		return;
	}

	if (TargetWidget->HandleInputEvent(InInputActionID, InValue))
	{
		UpdateCursorState();
		return;
	}

	if (InValue.m_blValue)
	{
		ToggleChildUI(TargetWidget);
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

bool ULxUIManager::IsManagedUIVisible(const ULxUIBaseObject* InChildUIWidget) const
{
	return InChildUIWidget && InChildUIWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& InChildUIWidget->GetVisibility() != ESlateVisibility::Hidden;
}

void ULxUIManager::UpdateTooltipPosition(ULxUIBaseObject* InChildUIWidget, FVector2D InAnchorScreenPosition)
{
	if (!InChildUIWidget)
	{
		return;
	}

	const FVector2D WidgetSize = InChildUIWidget->GetDesiredSize();
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

	UpdateManagedUIPosition(InChildUIWidget, FinalPos);
}

void ULxUIManager::UpdateCursorState() const
{
	if (!m_pPlayerController)
	{
		return;
	}

	for (const FLxManagedUIWidgetData& WidgetData : RegisteredChildWidgets)
	{
		if (WidgetData.bShowCursorWhenVisible && IsManagedUIVisible(WidgetData.UIWidget))
		{
			m_pPlayerController->ShowCursorFun();
			return;
		}
	}

	m_pPlayerController->HideCursorFun();
}
