#include "LxTrainingDummyDamageWidget.h"

#include "Components/TextBlock.h"

void ULxTrainingDummyDamageWidget::SetDamageHistory(const TArray<FLxTrainingDummyDamageRecord>& InDamageHistory)
{
	DamageHistory = InDamageHistory;
	if (DamageHistory.Num() > 10)
	{
		DamageHistory.SetNum(10);
	}
	RefreshDamageHistory();
}

void ULxTrainingDummyDamageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDamageHistory();
}

FText ULxTrainingDummyDamageWidget::GetDamageEntryText(const int32 Index) const
{
	if (Index < 0 || Index >= 10)
	{
		return FText::GetEmpty();
	}
	if (!DamageHistory.IsValidIndex(Index))
	{
		return FText::FromString(FString::Printf(TEXT("%02d   —"), Index + 1));
	}
	const FLxTrainingDummyDamageRecord& Record = DamageHistory[Index];
	return FText::FromString(FString::Printf(TEXT("%02d   %.2f%s"), Index + 1,
		Record.DamageValue, Record.bCriticalHit ? TEXT("  暴击") : TEXT("")));
}

FLinearColor ULxTrainingDummyDamageWidget::GetDamageEntryColor(const int32 Index) const
{
	// 使用较深的线性颜色，保证场景界面白色底板上的伤害数字具有清楚的对比度。
	if (DamageHistory.IsValidIndex(Index))
	{
		if (DamageHistory[Index].bCriticalHit)
		{
			return FLinearColor(0.040f, 0.010f, 0.001f, 1.0f);
		}
		if (Index == 0)
		{
			return FLinearColor(0.001f, 0.020f, 0.025f, 1.0f);
		}
		return FLinearColor(0.003f, 0.006f, 0.010f, 1.0f);
	}
	return FLinearColor(0.009f, 0.014f, 0.019f, 1.0f);
}

FSlateColor ULxTrainingDummyDamageWidget::GetDamageEntrySlateColor(const int32 Index) const
{
	return FSlateColor(GetDamageEntryColor(Index));
}

ESlateVisibility ULxTrainingDummyDamageWidget::GetDamageEntryVisibility(const int32 Index) const
{
	return Index >= 0 && Index < 10 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}

void ULxTrainingDummyDamageWidget::RefreshDamageHistory()
{
	if (DamageHistoryText)
	{
		FString Text = TEXT("最近 10 次伤害\n");
		for (int32 Index = 0; Index < 10; ++Index)
		{
			Text += GetDamageEntryText(Index).ToString();
			if (Index != 9)
			{
				Text += TEXT("\n");
			}
		}
		DamageHistoryText->SetText(FText::FromString(Text));
	}
	OnDamageHistoryUpdated(DamageHistory);
}
