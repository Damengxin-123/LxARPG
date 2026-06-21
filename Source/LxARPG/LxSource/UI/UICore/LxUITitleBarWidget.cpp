#include "LxUITitleBarWidget.h"

#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"

void ULxUITitleBarWidget::SetTargetUIObject(ULxUIBaseObject* InTargetUIObject)
{
	TargetUIObject = InTargetUIObject;
}

ULxUIBaseObject* ULxUITitleBarWidget::GetTargetUIObject() const
{
	return TargetUIObject;
}

bool ULxUITitleBarWidget::CloseTargetUI()
{
	if (!TargetUIObject)
	{
		return false;
	}

	TargetUIObject->CloseUIDisplay();
	return true;
}

FReply ULxUITitleBarWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bEnableDrag && TargetUIObject && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDraggingTitleBar = true;
		TargetUIObject->BeginUIDrag(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULxUITitleBarWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingTitleBar && TargetUIObject)
	{
		TargetUIObject->UpdateUIDrag(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULxUITitleBarWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingTitleBar && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (TargetUIObject)
		{
			TargetUIObject->EndUIDrag();
		}

		bIsDraggingTitleBar = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULxUITitleBarWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (bIsDraggingTitleBar && TargetUIObject)
	{
		TargetUIObject->EndUIDrag();
	}

	bIsDraggingTitleBar = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void ULxUITitleBarWidget::NativeDestruct()
{
	if (bIsDraggingTitleBar && TargetUIObject)
	{
		TargetUIObject->EndUIDrag();
	}

	bIsDraggingTitleBar = false;
	Super::NativeDestruct();
}