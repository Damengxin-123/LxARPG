// Fill out your copyright notice in the Description page of Project Settings.


#include "LxUIBaseObject.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/IUserListEntry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ListViewBase.h"
#include "Components/PanelWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"

#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"

void ULxUIBaseObject::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	m_pCharacterDataTransferComponent = CharacterDataTransferComponent;
}

void ULxUIBaseObject::NativeDestruct()
{
	if (ULxUIManager* UIManager = OwningUIManager.Get()) UIManager->UnmountRoleWindow(this);
	UnregisterAllInputActionReceives();
	Super::NativeDestruct();
}

FReply ULxUIBaseObject::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.IsAltDown())
	{
		if (ULxUIBaseObject* Window = GetOwningRoleWindow())
		{
			if (ULxUIManager* UIManager = Window->GetOwningUIManager()) UIManager->ActivateRoleWindow(Window);
		}
	}
	// 预览阶段只记录激活，继续把点击交给按钮、格子及蓝图布局中的具体控件。
	return FReply::Unhandled();
}

void ULxUIBaseObject::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	HandleInputEvent(InInputActionID, InValue);
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

ULxUIBaseObject* ULxUIBaseObject::GetOwningRoleWindow() const
{
	const UWidget* CurrentWidget = this;
	TSet<const UWidget*> VisitedWidgets;
	while (IsValid(CurrentWidget) && !VisitedWidgets.Contains(CurrentWidget))
	{
		VisitedWidgets.Add(CurrentWidget);
		if (const ULxUIBaseObject* CurrentUI = Cast<ULxUIBaseObject>(CurrentWidget))
		{
			if (ULxUIManager* UIManager = CurrentUI->GetOwningUIManager(); UIManager && UIManager->IsMountedRoleWindow(CurrentUI))
				return const_cast<ULxUIBaseObject*>(CurrentUI);
		}
		const UWidget* ParentWidget = CurrentWidget->GetParent();
		if (!ParentWidget)
		{
			// 虚拟列表生成的物品格子没有普通面板父级，通过列表接口回到背包或仓库布局。
			if (const IUserListEntry* ListEntry = Cast<IUserListEntry>(CurrentWidget)) ParentWidget = ListEntry->GetOwningListView();
		}
		if (!ParentWidget)
		{
			// 控件树根没有面板父级，通过所属用户控件继续向上查找业务窗口。
			const UWidgetTree* OwningTree = CurrentWidget->GetTypedOuter<UWidgetTree>();
			ParentWidget = OwningTree ? Cast<UUserWidget>(OwningTree->GetOuter()) : nullptr;
		}
		CurrentWidget = ParentWidget;
	}
	return nullptr;
}

void ULxUIBaseObject::BeginUIDrag(FVector2D InMouseScreenPosition)
{
	if (ULxUIBaseObject* Window = GetOwningRoleWindow())
	{
		if (ULxUIManager* UIManager = Window->GetOwningUIManager()) UIManager->ActivateRoleWindow(Window);
	}
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
