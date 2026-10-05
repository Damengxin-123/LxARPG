#include "LxUITitleBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"

void ULxUITitleBarWidget::SetTargetUIObject(ULxUIBaseObject* InTargetUIObject)
{
	TargetUIObject = InTargetUIObject;
}

ULxUIBaseObject* ULxUITitleBarWidget::GetTargetUIObject() const
{
	if (IsValid(TargetUIObject))
	{
		return TargetUIObject;
	}

	const UWidget* CurrentWidget = this;
	TSet<const UWidget*> VisitedWidgets;
	while (IsValid(CurrentWidget) && !VisitedWidgets.Contains(CurrentWidget))
	{
		VisitedWidgets.Add(CurrentWidget);
		UWidget* ParentWidget = CurrentWidget->GetParent();
		if (!ParentWidget)
		{
			// 布局根节点没有面板父级，需通过所属控件树跨越用户控件边界。
			const UWidgetTree* OwningTree = CurrentWidget->GetTypedOuter<UWidgetTree>();
			ParentWidget = OwningTree ? Cast<UUserWidget>(OwningTree->GetOuter()) : nullptr;
		}

		if (ULxUIBaseObject* ParentUI = Cast<ULxUIBaseObject>(ParentWidget); IsValid(ParentUI))
		{
			return ParentUI;
		}
		CurrentWidget = ParentWidget;
	}

	return nullptr;
}

bool ULxUITitleBarWidget::CloseTargetUI()
{
	ULxUIBaseObject* TargetUI = GetTargetUIObject();
	if (!TargetUI)
	{
		return false;
	}

	EndTitleBarDrag();
	TargetUI->CloseUIDisplay();
	return true;
}

FReply ULxUITitleBarWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bEnableDrag && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (ULxUIBaseObject* TargetUI = GetTargetUIObject())
		{
			DragTargetUIObject = TargetUI;
			bIsDraggingTitleBar = true;
			TargetUI->BeginUIDrag(InMouseEvent.GetScreenSpacePosition());
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULxUITitleBarWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ULxUIBaseObject* DragTarget = DragTargetUIObject.Get(); bIsDraggingTitleBar && DragTarget)
	{
		DragTarget->UpdateUIDrag(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULxUITitleBarWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingTitleBar && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		EndTitleBarDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULxUITitleBarWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	EndTitleBarDrag();
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void ULxUITitleBarWidget::NativeDestruct()
{
	EndTitleBarDrag();
	Super::NativeDestruct();
}

void ULxUITitleBarWidget::EndTitleBarDrag()
{
	if (ULxUIBaseObject* DragTarget = DragTargetUIObject.Get(); bIsDraggingTitleBar && DragTarget)
	{
		DragTarget->EndUIDrag();
	}

	bIsDraggingTitleBar = false;
	DragTargetUIObject.Reset();
}
