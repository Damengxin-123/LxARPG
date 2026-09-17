#include "LxInteractionEntranceWidget.h"

#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/UI/Option/LxOptionViewData.h"

void ULxInteractionEntranceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindPlayerInteractionComponent();
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->RefreshEntranceOptions();
	}
	UpdateEntranceVisibilityAndInputRegistration();
}

void ULxInteractionEntranceWidget::NativeDestruct()
{
	if (bIsInteractionInputRegistered)
	{
		UnregisterInputActionReceive(InteractionTriggerInputActionID);
		bIsInteractionInputRegistered = false;
	}

	UnbindPlayerInteractionComponent();
	InvalidateEntranceOptionCallbacks();
	CachedEntranceOptions.Reset();
	CurrentEntranceOptionIndex = INDEX_NONE;
	Super::NativeDestruct();
}

bool ULxInteractionEntranceWidget::HandleInputEvent(ELxInputActionID InputID, const FLxInputValue& Value)
{
	if (InputID != InteractionTriggerInputActionID || !Value.m_blValue || !ShouldShowEntrance())
	{
		return false;
	}

	HandleInteractionTriggerKeyPressed();
	return true;
}

void ULxInteractionEntranceWidget::SetPlayerInteractionComponent(ULxPlayerInteractionModule* InPlayerInteractionComponent)
{
	if (PlayerInteractionComponent == InPlayerInteractionComponent)
	{
		return;
	}

	UnbindPlayerInteractionComponent();
	PlayerInteractionComponent = InPlayerInteractionComponent;
	BindPlayerInteractionComponent();

	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->RefreshEntranceOptions();
	}
	else
	{
		HandleEntranceOptionsUpdated({});
	}
}

FText ULxInteractionEntranceWidget::GetEntranceOptionPromptText(int32 OptionIndex) const
{
	return CachedEntranceOptions.IsValidIndex(OptionIndex) ? CachedEntranceOptions[OptionIndex]->OptionText : FText();
}

void ULxInteractionEntranceWidget::SubmitEntranceOptionIndex(int32 OptionIndex)
{
	if (!ShouldShowEntrance() || !CachedEntranceOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	CurrentEntranceOptionIndex = OptionIndex;
	PlayerInteractionComponent->SelectEntranceOptionByIndex(CurrentEntranceOptionIndex);
	UpdateEntranceVisibilityAndInputRegistration();
	if (ShouldShowEntrance())
	{
		RefreshEntranceOptionSelection();
	}
}

void ULxInteractionEntranceWidget::SubmitCurrentEntranceOption()
{
	SubmitEntranceOptionIndex(GetCurrentEntranceOptionIndex());
}

void ULxInteractionEntranceWidget::ScrollEntrancePromptTexts(float MouseWheelValue)
{
	if (!ShouldShowEntrance() || !FMath::IsFinite(MouseWheelValue) || FMath::IsNearlyZero(MouseWheelValue))
	{
		return;
	}

	const int32 NewOptionIndex = FMath::Clamp(
		CurrentEntranceOptionIndex + (MouseWheelValue < 0.0f ? 1 : -1), 0, CachedEntranceOptions.Num() - 1);
	if (NewOptionIndex == CurrentEntranceOptionIndex)
	{
		return;
	}

	CurrentEntranceOptionIndex = NewOptionIndex;
	RefreshEntranceOptionSelection();
}

void ULxInteractionEntranceWidget::RefreshEntranceOptionSelection()
{
	InvalidateEntranceOptionCallbacks();
	for (int32 OptionIndex = 0; OptionIndex < CachedEntranceOptions.Num(); ++OptionIndex)
	{
		CachedEntranceOptions[OptionIndex] = CreateEntranceOptionViewData(
			CachedEntranceOptions[OptionIndex]->OptionText, OptionIndex);
	}
	BroadcastEntranceOptionViewDataUpdated();
}

void ULxInteractionEntranceWidget::HandleInteractionTriggerKeyPressed_Implementation()
{
	SubmitCurrentEntranceOption();
}

void ULxInteractionEntranceWidget::BindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnEntranceOptionsUpdated.RemoveDynamic(this, &ULxInteractionEntranceWidget::HandleEntranceOptionsUpdated);
	PlayerInteractionComponent->OnEntranceOptionsUpdated.AddDynamic(this, &ULxInteractionEntranceWidget::HandleEntranceOptionsUpdated);
	PlayerInteractionComponent->OnInteractionOptionActivated.AddUniqueDynamic(this, &ULxInteractionEntranceWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.AddUniqueDynamic(this, &ULxInteractionEntranceWidget::HandleInteractionCancelled);
}

void ULxInteractionEntranceWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnEntranceOptionsUpdated.RemoveDynamic(this, &ULxInteractionEntranceWidget::HandleEntranceOptionsUpdated);
	PlayerInteractionComponent->OnInteractionOptionActivated.RemoveDynamic(this, &ULxInteractionEntranceWidget::HandleInteractionOptionActivated);
	PlayerInteractionComponent->OnInteractionCancelled.RemoveDynamic(this, &ULxInteractionEntranceWidget::HandleInteractionCancelled);
}

void ULxInteractionEntranceWidget::HandleEntranceOptionsUpdated(const TArray<FLxInteractionOption>& Options)
{
	InvalidateEntranceOptionCallbacks();
	CachedEntranceOptions.Reset(Options.Num());
	CurrentEntranceOptionIndex = Options.IsEmpty() ? INDEX_NONE : 0;
	for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
	{
		CachedEntranceOptions.Add(CreateEntranceOptionViewData(Options[OptionIndex].PromptText, OptionIndex));
	}
	UpdateEntranceVisibilityAndInputRegistration();
	BroadcastEntranceOptionViewDataUpdated();
}

ULxOptionViewData* ULxInteractionEntranceWidget::CreateEntranceOptionViewData(const FText& Text, int32 OptionIndex)
{
	ULxOptionViewData* Data = NewObject<ULxOptionViewData>(this);
	Data->OptionText = Text;
	Data->OptionIndex = OptionIndex;
	Data->bSelected = OptionIndex == CurrentEntranceOptionIndex;
	Data->OnOptionTriggered.BindUObject(this, &ULxInteractionEntranceWidget::SubmitEntranceOptionIndex);
	return Data;
}

void ULxInteractionEntranceWidget::InvalidateEntranceOptionCallbacks()
{
	for (ULxOptionViewData* Data : CachedEntranceOptions)
	{
		Data->OnOptionTriggered.Unbind();
	}
}

void ULxInteractionEntranceWidget::BroadcastEntranceOptionViewDataUpdated()
{
	// 使用独立数组，蓝图回调重建入口列表时不会改变本次事件参数。
	const TArray<ULxOptionViewData*> Options = CachedEntranceOptions;
	OnEntranceOptionViewDataUpdated(Options);
}

bool ULxInteractionEntranceWidget::ShouldShowEntrance() const
{
	return PlayerInteractionComponent && PlayerInteractionComponent->CanSelectEntranceOption()
		&& !CachedEntranceOptions.IsEmpty();
}

void ULxInteractionEntranceWidget::HandleInteractionOptionActivated(
	const FLxInteractionOption& Option, ELxInteractionActionType InteractionType)
{
	UpdateEntranceVisibilityAndInputRegistration();
}

void ULxInteractionEntranceWidget::HandleInteractionCancelled()
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->RefreshEntranceOptions();
	}
}

void ULxInteractionEntranceWidget::UpdateEntranceVisibilityAndInputRegistration()
{
	const bool bShouldShowEntrance = ShouldShowEntrance();
	SetVisibility(bShouldShowEntrance ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bShouldShowEntrance && !bIsInteractionInputRegistered)
	{
		RegisterInputActionReceive(InteractionTriggerInputActionID);
		bIsInteractionInputRegistered = true;
	}
	else if (!bShouldShowEntrance && bIsInteractionInputRegistered)
	{
		UnregisterInputActionReceive(InteractionTriggerInputActionID);
		bIsInteractionInputRegistered = false;
	}
}
