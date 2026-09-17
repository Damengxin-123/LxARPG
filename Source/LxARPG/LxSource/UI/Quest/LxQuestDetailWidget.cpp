#include "LxQuestDetailWidget.h"

#include "LxARPG/LxSource/UI/Option/LxOptionViewData.h"

void ULxQuestDetailWidget::NativeConstruct()
{
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeConstruct();
}

void ULxQuestDetailWidget::NativeDestruct()
{
	InvalidateOptions();
	Super::NativeDestruct();
}

void ULxQuestDetailWidget::SetVisibility(ESlateVisibility InVisibility)
{
	const bool bOpening = !IsVisible() && InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden;
	Super::SetVisibility(InVisibility);
	if (bOpening)
	{
		SelectedIndex = INDEX_NONE;
		RefreshQuestDisplay();
	}
}

void ULxQuestDetailWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* Component)
{
	if (m_pCharacterDataTransferComponent != Component)
	{
		SelectedIndex = INDEX_NONE;
	}
	Super::UpdateUIComponents(Component);
}

void ULxQuestDetailWidget::RefreshQuestDisplay()
{
	const FLxQuestRuntimeRecord Previous = Details.IsValidIndex(SelectedIndex) ? Details[SelectedIndex].Progress : FLxQuestRuntimeRecord();
	Details = ReadQuestDetails();
	SelectedIndex = Details.IndexOfByPredicate([&Previous](const FLxQuestDetailViewData& Detail)
	{
		return Detail.Progress.QuestSeriesId == Previous.QuestSeriesId && Detail.Progress.QuestId == Previous.QuestId;
	});
	if (SelectedIndex == INDEX_NONE && !Details.IsEmpty())
	{
		SelectedIndex = 0;
	}
	PublishSelection();
}

void ULxQuestDetailWidget::SelectQuest(int32 OptionIndex)
{
	if (!IsVisible() || !Details.IsValidIndex(OptionIndex))
	{
		return;
	}
	SelectedIndex = OptionIndex;
	PublishSelection();
}

void ULxQuestDetailWidget::InvalidateOptions()
{
	for (ULxOptionViewData* Option : Options)
	{
		Option->OnOptionTriggered.Unbind();
	}
}

void ULxQuestDetailWidget::PublishSelection()
{
	InvalidateOptions();
	Options.Reset(Details.Num());
	for (int32 Index = 0; Index < Details.Num(); ++Index)
	{
		ULxOptionViewData* Option = NewObject<ULxOptionViewData>(this);
		Option->OptionIndex = Index;
		Option->OptionText = Details[Index].Definition.DisplayName;
		Option->bSelected = Index == SelectedIndex;
		Option->OnOptionTriggered.BindUObject(this, &ULxQuestDetailWidget::SelectQuest);
		Options.Add(Option);
	}
	const TArray<ULxOptionViewData*> Snapshot = Options;
	const bool bHasSelection = Details.IsValidIndex(SelectedIndex);
	const FLxQuestDetailViewData Detail = bHasSelection ? Details[SelectedIndex] : FLxQuestDetailViewData();
	OnQuestListUpdated(Snapshot);
	OnQuestDetailUpdated(bHasSelection, Detail);
}
