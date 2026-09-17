#include "LxQuestSummaryListItemWidget.h"

#include "LxQuestSummaryViewData.h"

void ULxQuestSummaryListItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	const ULxQuestSummaryViewData* Item = Cast<ULxQuestSummaryViewData>(ListItemObject);
	OnQuestSummaryItemUpdated(Item ? Item->QuestName : FText(), Item ? Item->State : ELxQuestRuntimeState::NotAccepted);
}

void ULxQuestSummaryListItemWidget::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();
	OnQuestSummaryItemUpdated(FText(), ELxQuestRuntimeState::NotAccepted);
}
