#include "LxShortcutBarWidget.h"

#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

namespace
{
	int32 ShortcutInputActionToIndex(ELxInputActionID InInputActionID)
	{
		return static_cast<int32>(InInputActionID) - static_cast<int32>(ELxInputActionID::Shortcut0);
	}
}

void ULxShortcutBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterShortcutInputActions();
}

void ULxShortcutBarWidget::NativeDestruct()
{
	StopRepeatedUseTimer();
	ClearShortcutSelection();
	Super::NativeDestruct();
}

void ULxShortcutBarWidget::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (IsShortcutInputActionID(InInputActionID))
	{
		if (InValue.m_blValue)
		{
			HandleShortcutPressed(InInputActionID);
		}
		return;
	}

	if (InInputActionID == ELxInputActionID::ShortcutUse)
	{
		if (InValue.m_blValue)
		{
			BeginUseSelectedShortcut();
		}
		else
		{
			EndUseSelectedShortcut();
		}
		return;
	}

	if (InInputActionID == ELxInputActionID::ShortcutSwitch)
	{
		SelectShortcutByWheel(InValue.m_nVector1D);
	}
}

bool ULxShortcutBarWidget::BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID)
{
	if (!SetupShortcutItemGrid(InItemGridWidget, InInputActionID))
	{
		return false;
	}

	ShortcutGridMap.Add(InInputActionID, InItemGridWidget);

	const int32 TargetIndex = ShortcutInputActionToIndex(InInputActionID);
	if (TargetIndex >= 0)
	{
		OrderedShortcutGrids.SetNum(FMath::Max(OrderedShortcutGrids.Num(), TargetIndex + 1));
		OrderedShortcutGrids[TargetIndex] = InItemGridWidget;
	}

	RegisterShortcutInputActions();
	return true;
}

bool ULxShortcutBarWidget::SelectShortcutGrid(ULxItemGridWidget* InItemGridWidget)
{
	if (!InItemGridWidget || !InItemGridWidget->ItemIsVaild())
	{
		return false;
	}

	if (SelectedShortcutGrid == InItemGridWidget)
	{
		SelectedShortcutGrid->SetShortcutSelected(true);
		return true;
	}

	EndUseSelectedShortcut();
	if (SelectedShortcutGrid)
	{
		SelectedShortcutGrid->SetShortcutSelected(false);
	}

	SelectedShortcutGrid = InItemGridWidget;
	SelectedShortcutGrid->SetShortcutSelected(true);
	return true;
}

void ULxShortcutBarWidget::ClearShortcutSelection()
{
	EndUseSelectedShortcut();
	if (SelectedShortcutGrid)
	{
		SelectedShortcutGrid->SetShortcutSelected(false);
		SelectedShortcutGrid = nullptr;
	}
}

bool ULxShortcutBarWidget::SelectShortcutByWheel(float InWheelValue)
{
	if (FMath::IsNearlyZero(InWheelValue))
	{
		return false;
	}

	return SelectShortcutByOffset(InWheelValue > 0.0f ? -1 : 1);
}

bool ULxShortcutBarWidget::BeginUseSelectedShortcut()
{
	if (!SelectedShortcutGrid || !SelectedShortcutGrid->ItemIsVaild())
	{
		return false;
	}

	ULxItemBase* SelectedItem = SelectedShortcutGrid->GetCurrentItem();
	if (!SelectedItem)
	{
		return false;
	}

	if (SelectedItem->ItemType() != ELxItemType::Skill)
	{
		bUsingSelectedShortcut = SelectedShortcutGrid->UseItem();
		return bUsingSelectedShortcut;
	}

	ULxSkillItem* SkillItem = SelectedShortcutGrid->GetCurrentSkillItem();
	ULxSkill* Skill = SkillItem ? SkillItem->GetOrCreateSkillObject() : nullptr;
	if (!Skill)
	{
		return false;
	}

	bUsingSelectedShortcut = true;
	SetMoveRotationLockedByShortcut(true);
	if (Skill->CanSkillCharge())
	{
		bChargingSelectedShortcut = SelectedShortcutGrid->StartUseItem();
		bUsingSelectedShortcut = bChargingSelectedShortcut;
		if (!bUsingSelectedShortcut)
		{
			SetMoveRotationLockedByShortcut(false);
		}
		return bChargingSelectedShortcut;
	}

	UseSelectedShortcutRepeatedly();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RepeatedUseTimerHandle,
			this,
			&ULxShortcutBarWidget::UseSelectedShortcutRepeatedly,
			Skill->GetEffectiveReleaseCooldown(),
			true);
	}
	return true;
}

bool ULxShortcutBarWidget::EndUseSelectedShortcut()
{
	StopRepeatedUseTimer();
	if (bChargingSelectedShortcut && SelectedShortcutGrid)
	{
		SelectedShortcutGrid->EndUseItem();
	}

	SetMoveRotationLockedByShortcut(false);
	bUsingSelectedShortcut = false;
	bChargingSelectedShortcut = false;
	return true;
}

bool ULxShortcutBarWidget::SetupShortcutItemGrid(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID)
{
	if (!InItemGridWidget || !IsShortcutInputActionID(InInputActionID))
	{
		return false;
	}

	ULxItemSlotData* ShortcutSlot = NewObject<ULxItemSlotData>(InItemGridWidget);
	ShortcutSlot->InitItemSlot(ELxItemSlotType::Shortcut);

	ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(InItemGridWidget);
	ItemUIData->m_pSlotData = ShortcutSlot;

	InItemGridWidget->NativeOnListItemObjectSet(ItemUIData);
	return true;
}

FReply ULxShortcutBarWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return BeginUseSelectedShortcut() ? FReply::Handled().CaptureMouse(TakeWidget()) : FReply::Unhandled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULxShortcutBarWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return EndUseSelectedShortcut() ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply ULxShortcutBarWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return SelectShortcutByWheel(InMouseEvent.GetWheelDelta()) ? FReply::Handled() : Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void ULxShortcutBarWidget::RegisterShortcutInputActions()
{
	for (int32 Index = static_cast<int32>(ELxInputActionID::Shortcut0); Index <= static_cast<int32>(ELxInputActionID::Shortcut9); ++Index)
	{
		RegisterInputActionReceive(static_cast<ELxInputActionID>(Index));
	}

	RegisterInputActionReceive(ELxInputActionID::ShortcutUse);
	RegisterInputActionReceive(ELxInputActionID::ShortcutSwitch);
}

bool ULxShortcutBarWidget::HandleShortcutPressed(ELxInputActionID InInputActionID)
{
	ULxItemGridWidget* ShortcutGrid = GetShortcutGridByInputAction(InInputActionID);
	if (!ShortcutGrid || !ShortcutGrid->ItemIsVaild())
	{
		return false;
	}

	switch (ShortcutGrid->GetItemType())
	{
	case ELxItemType::Skill:
		return SelectShortcutGrid(ShortcutGrid);
	case ELxItemType::Consumable:
		return ShortcutGrid->UseItem();
	default:
		return false;
	}
}

bool ULxShortcutBarWidget::SelectShortcutByOffset(int32 InOffset)
{
	if (OrderedShortcutGrids.IsEmpty() || InOffset == 0)
	{
		return false;
	}

	const int32 CurrentIndex = GetShortcutGridIndex(SelectedShortcutGrid);
	const int32 StartIndex = CurrentIndex == INDEX_NONE ? (InOffset > 0 ? -1 : OrderedShortcutGrids.Num()) : CurrentIndex;
	for (int32 Step = 1; Step <= OrderedShortcutGrids.Num(); ++Step)
	{
		const int32 CandidateIndex = (StartIndex + InOffset * Step + OrderedShortcutGrids.Num()) % OrderedShortcutGrids.Num();
		ULxItemGridWidget* CandidateGrid = OrderedShortcutGrids[CandidateIndex];
		if (CandidateGrid && CandidateGrid->ItemIsVaild())
		{
			return SelectShortcutGrid(CandidateGrid);
		}
	}

	return false;
}

void ULxShortcutBarWidget::UseSelectedShortcutRepeatedly()
{
	if (!bUsingSelectedShortcut
		|| !SelectedShortcutGrid
		|| !SelectedShortcutGrid->ItemIsVaild()
		|| SelectedShortcutGrid->GetItemType() != ELxItemType::Skill)
	{
		StopRepeatedUseTimer();
		return;
	}

	SelectedShortcutGrid->UseItem();
}

void ULxShortcutBarWidget::StopRepeatedUseTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepeatedUseTimerHandle);
	}
}

void ULxShortcutBarWidget::SetMoveRotationLockedByShortcut(bool bInLocked)
{
	if (bMoveRotationLockedByShortcut == bInLocked)
	{
		return;
	}

	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwningPlayerPawn());
	ULxCharacterMoveComponent* MoveComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMoveComponent() : nullptr;
	if (!MoveComponent)
	{
		return;
	}

	bMoveRotationLockedByShortcut = bInLocked;
	if (bMoveRotationLockedByShortcut)
	{
		MoveComponent->AddMoveRotationLock();
	}
	else
	{
		MoveComponent->RemoveMoveRotationLock();
	}
}

ULxItemGridWidget* ULxShortcutBarWidget::GetShortcutGridByInputAction(ELxInputActionID InInputActionID) const
{
	return ShortcutGridMap.FindRef(InInputActionID);
}

int32 ULxShortcutBarWidget::GetShortcutGridIndex(ULxItemGridWidget* InItemGridWidget) const
{
	if (!InItemGridWidget)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < OrderedShortcutGrids.Num(); ++Index)
	{
		if (OrderedShortcutGrids[Index] == InItemGridWidget)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool ULxShortcutBarWidget::IsShortcutInputActionID(ELxInputActionID InInputActionID)
{
	return InInputActionID >= ELxInputActionID::Shortcut0
		&& InInputActionID <= ELxInputActionID::Shortcut9;
}
