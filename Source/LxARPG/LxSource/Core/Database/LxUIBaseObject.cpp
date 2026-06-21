// Fill out your copyright notice in the Description page of Project Settings.


#include "LxUIBaseObject.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"

void ULxUIBaseObject::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	m_pCharacterDataTransferComponent = CharacterDataTransferComponent;
}

void ULxUIBaseObject::NativeConstruct()
{
	Super::NativeConstruct();
}

void ULxUIBaseObject::NativeDestruct()
{
	UnregisterAllInputActionReceives();
	Super::NativeDestruct();
}

void ULxUIBaseObject::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	HandleInputEvent(InInputActionID, InValue);
}

void ULxUIBaseObject::InitMonitorRegistration()
{
}

void ULxUIBaseObject::RegisterInputActionReceive(ELxInputActionID InInputActionID)
{
	if (InInputActionID == ELxInputActionID::None || RegisteredInputActionIDs.Contains(InInputActionID))
	{
		return;
	}

	TScriptInterface<ILxInputReceiveInterface> InputReceive;
	InputReceive.SetObject(this);
	InputReceive.SetInterface(Cast<ILxInputReceiveInterface>(this));
	LxInputActionConfig::RegisterInputReceive(InInputActionID, InputReceive);
	RegisteredInputActionIDs.Add(InInputActionID);
}

void ULxUIBaseObject::UnregisterInputActionReceive(ELxInputActionID InInputActionID)
{
	if (InInputActionID == ELxInputActionID::None)
	{
		return;
	}

	LxInputActionConfig::UnregisterInputReceive(InInputActionID, this);
	RegisteredInputActionIDs.Remove(InInputActionID);
}

void ULxUIBaseObject::UnregisterAllInputActionReceives()
{
	for (const ELxInputActionID InputActionID : RegisteredInputActionIDs)
	{
		LxInputActionConfig::UnregisterInputReceive(InputActionID, this);
	}

	RegisteredInputActionIDs.Reset();
}

void ULxUIBaseObject::SetOwningUIManager(ULxUIManager* InOwningUIManager)
{
	OwningUIManager = InOwningUIManager;
}

ULxUIManager* ULxUIBaseObject::GetOwningUIManager() const
{
	return OwningUIManager.Get();
}

void ULxUIBaseObject::BeginUIDrag(FVector2D InMouseScreenPosition)
{
	bIsUIDragging = true;
	LastUIDragMouseScreenPosition = InMouseScreenPosition;
}

void ULxUIBaseObject::UpdateUIDrag(FVector2D InMouseScreenPosition)
{
	if (!bIsUIDragging)
	{
		return;
	}

	const FVector2D DragDelta = InMouseScreenPosition - LastUIDragMouseScreenPosition;
	LastUIDragMouseScreenPosition = InMouseScreenPosition;
	DragUIByDelta(DragDelta);
}

void ULxUIBaseObject::DragUIByDelta(FVector2D InDragDelta)
{
	if (InDragDelta.IsNearlyZero())
	{
		return;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ScaledDragDelta = FMath::IsNearlyZero(ViewportScale) ? InDragDelta : InDragDelta / ViewportScale;

	if (UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasPanelSlot->SetPosition(CanvasPanelSlot->GetPosition() + ScaledDragDelta);
		return;
	}

	SetRenderTranslation(GetRenderTransform().Translation + ScaledDragDelta);
}

void ULxUIBaseObject::EndUIDrag()
{
	bIsUIDragging = false;
	LastUIDragMouseScreenPosition = FVector2D::ZeroVector;
}

void ULxUIBaseObject::CloseUIDisplay()
{
	EndUIDrag();

	if (ULxUIManager* UIManager = OwningUIManager.Get())
	{
		UIManager->SetChildUIVisible(this, false);
		return;
	}

	SetVisibility(ESlateVisibility::Collapsed);
}
