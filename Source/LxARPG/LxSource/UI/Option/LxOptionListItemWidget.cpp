#include "LxOptionListItemWidget.h"

#include "LxOptionViewData.h"

void ULxOptionListItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	OptionData = Cast<ULxOptionViewData>(ListItemObject);
	RefreshOptionDisplay();
}

void ULxOptionListItemWidget::NativeOnEntryReleased()
{
	OptionData = nullptr;
	RefreshOptionDisplay();
	IUserObjectListEntry::NativeOnEntryReleased();
}

void ULxOptionListItemWidget::TriggerOption()
{
	if (OptionData != nullptr)
	{
		// 复制回调，允许接收方在执行期间重建列表或重新绑定数据中的回调。
		const FOnLxOptionTriggered Callback = OptionData->OnOptionTriggered;
		const int32 OptionIndex = OptionData->OptionIndex;
		Callback.ExecuteIfBound(OptionIndex);
	}
}

void ULxOptionListItemWidget::RefreshOptionDisplay()
{
	if (OptionData != nullptr)
	{
		OnOptionDisplayUpdated(OptionData->OptionText, OptionData->bSelected);
	}
	else
	{
		OnOptionDisplayUpdated(FText::GetEmpty(), false);
	}
}
