#include "LxInteractionEntranceWidget.h"

#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"

void ULxInteractionEntranceWidget::NativeConstruct()
{
	Super::NativeConstruct();
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
		CachedEntranceOptions.Reset();
		ResetPromptTextTagTables();
		BroadcastPromptTextTagTablesUpdated();
		UpdateEntranceVisibilityAndInputRegistration();
	}
}

FGameplayTag ULxInteractionEntranceWidget::GetEntranceOptionPromptTextTag(int32 OptionIndex) const
{
	return CachedEntranceOptions.IsValidIndex(OptionIndex) ? CachedEntranceOptions[OptionIndex].PromptTextTag : FGameplayTag();
}

FGameplayTag ULxInteractionEntranceWidget::GetEntranceOptionIDTag(int32 OptionIndex) const
{
	return CachedEntranceOptions.IsValidIndex(OptionIndex) ? CachedEntranceOptions[OptionIndex].InteractionIDTag : FGameplayTag();
}

int32 ULxInteractionEntranceWidget::GetCurrentEntranceOptionIndex() const
{
	return CachedCurrentAndLowerPromptTextTags.Num() > 0 ? CachedUpperPromptTextTags.Num() : INDEX_NONE;
}

void ULxInteractionEntranceWidget::SubmitEntranceOptionIndex(int32 OptionIndex)
{
	if (!PlayerInteractionComponent || !CachedEntranceOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FLxInteractionOption SelectedOption = CachedEntranceOptions[OptionIndex];
	if (PlayerInteractionComponent->ActivateInteractionOption(SelectedOption))
	{
		if (ShouldHideEntranceAfterSelection(SelectedOption))
		{
			HideEntranceAfterSelection();
		}
		else
		{
			UpdateEntranceVisibilityAndInputRegistration();
		}
	}
}

void ULxInteractionEntranceWidget::SubmitCurrentEntranceOption()
{
	SubmitEntranceOptionIndex(GetCurrentEntranceOptionIndex());
}

void ULxInteractionEntranceWidget::ScrollEntrancePromptTextTags(float MouseWheelValue)
{
	if (FMath::IsNearlyZero(MouseWheelValue))
	{
		return;
	}

	if (MouseWheelValue < 0.0f)
	{
		// 向下滚动时，下方表第一个元素从“当前项”变为“上方最后一项”。
		if (CachedCurrentAndLowerPromptTextTags.Num() <= 1)
		{
			return;
		}

		CachedUpperPromptTextTags.Add(CachedCurrentAndLowerPromptTextTags[0]);
		CachedCurrentAndLowerPromptTextTags.RemoveAt(0);
	}
	else
	{
		// 向上滚动时，上方表最后一个元素回到下方表开头，成为当前项。
		if (CachedUpperPromptTextTags.IsEmpty())
		{
			return;
		}

		const FGameplayTag PreviousPromptTextTag = CachedUpperPromptTextTags.Last();
		CachedUpperPromptTextTags.RemoveAt(CachedUpperPromptTextTags.Num() - 1);
		CachedCurrentAndLowerPromptTextTags.Insert(PreviousPromptTextTag, 0);
	}

	BroadcastPromptTextTagTablesUpdated();
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
}

void ULxInteractionEntranceWidget::UnbindPlayerInteractionComponent()
{
	if (!PlayerInteractionComponent)
	{
		return;
	}

	PlayerInteractionComponent->OnEntranceOptionsUpdated.RemoveDynamic(this, &ULxInteractionEntranceWidget::HandleEntranceOptionsUpdated);
}

void ULxInteractionEntranceWidget::HandleEntranceOptionsUpdated(const TArray<FLxInteractionOption>& Options)
{
	CachedEntranceOptions = Options;
	ResetPromptTextTagTables();
	BroadcastPromptTextTagTablesUpdated();
	UpdateEntranceVisibilityAndInputRegistration();
}

void ULxInteractionEntranceWidget::ResetPromptTextTagTables()
{
	CachedUpperPromptTextTags.Reset();
	CachedCurrentAndLowerPromptTextTags.Reset();
	CachedCurrentAndLowerPromptTextTags.Reserve(CachedEntranceOptions.Num());

	// 新入口列表默认选中第一项，因此全部标签先进入“当前及下方”表。
	for (const FLxInteractionOption& Option : CachedEntranceOptions)
	{
		CachedCurrentAndLowerPromptTextTags.Add(Option.PromptTextTag);
	}
}

void ULxInteractionEntranceWidget::BroadcastPromptTextTagTablesUpdated()
{
	OnEntrancePromptTextTagsUpdated(CachedUpperPromptTextTags, CachedCurrentAndLowerPromptTextTags);
}

bool ULxInteractionEntranceWidget::ShouldShowEntrance() const
{
	return CachedEntranceOptions.Num() > 0;
}

bool ULxInteractionEntranceWidget::ShouldHideEntranceAfterSelection(const FLxInteractionOption& Option) const
{
	return Option.InteractionType != ELxInteractionActionType::TriggerMechanism ||
		!Option.InteractionNode ||
		Option.InteractionNode->GetParentNode() != nullptr;
}

void ULxInteractionEntranceWidget::HideEntranceAfterSelection()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (bIsInteractionInputRegistered)
	{
		UnregisterInputActionReceive(InteractionTriggerInputActionID);
		bIsInteractionInputRegistered = false;
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
