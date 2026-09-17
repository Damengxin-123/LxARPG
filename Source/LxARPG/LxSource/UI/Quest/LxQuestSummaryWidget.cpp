#include "LxQuestSummaryWidget.h"

#include "LxQuestSummaryViewData.h"

void ULxQuestSummaryWidget::RefreshQuestDisplay()
{
	SummaryItems.Reset();
	for (const FLxQuestDetailViewData& Detail : ReadQuestDetails())
	{
		if (Detail.Progress.State != ELxQuestRuntimeState::ReadyToSubmit && Detail.Progress.State != ELxQuestRuntimeState::InProgress)
		{
			continue;
		}
		ULxQuestSummaryViewData* Item = NewObject<ULxQuestSummaryViewData>(this);
		Item->QuestName = Detail.Definition.DisplayName;
		Item->State = Detail.Progress.State;
		SummaryItems.Add(Item);
	}
	const TArray<ULxQuestSummaryViewData*> Snapshot = SummaryItems;
	OnQuestSummaryUpdated(Snapshot);
}
