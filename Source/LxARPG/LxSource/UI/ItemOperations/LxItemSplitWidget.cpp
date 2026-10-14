#include "LxItemSplitWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "LxItemOperationUIRouter.h"

void ULxItemSplitWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    if (CountInput)
    {
        CountInput->OnValueChanged.RemoveDynamic(this, &ULxItemSplitWidget::HandleInputChanged);
        CountInput->OnValueChanged.AddDynamic(this, &ULxItemSplitWidget::HandleInputChanged);
    }
    if (CountSlider)
    {
        CountSlider->OnValueChanged.RemoveDynamic(this, &ULxItemSplitWidget::HandleSliderChanged);
        CountSlider->OnValueChanged.AddDynamic(this, &ULxItemSplitWidget::HandleSliderChanged);
    }
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.RemoveDynamic(this, &ULxItemSplitWidget::ConfirmSplit);
        ConfirmButton->OnClicked.AddDynamic(this, &ULxItemSplitWidget::ConfirmSplit);
    }
    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveDynamic(this, &ULxItemSplitWidget::CancelSplit);
        CancelButton->OnClicked.AddDynamic(this, &ULxItemSplitWidget::CancelSplit);
    }
    RefreshControls();
}

void ULxItemSplitWidget::InitializeSplit(ULxItemOperationUIRouter* InRouter, const FText& ItemName, int32 TotalCount)
{
    Router = InRouter;
    MaximumCount = FMath::Max(1, TotalCount - 1);
    SplitCount = FMath::Clamp(TotalCount / 2, 1, MaximumCount);
    bPending = false;
    StatusMessage = FText::GetEmpty();
    if (ItemNameText) ItemNameText->SetText(ItemName);
    RefreshControls();
}

void ULxItemSplitWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bPending && Router.IsValid() && !Router->IsSplitSourceValid()) Router->CancelSplit();
}

FReply ULxItemSplitWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (!bPending) CancelSplit();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULxItemSplitWidget::SetSplitCount(int32 InCount)
{
    if (bPending) return;
    SplitCount = FMath::Clamp(InCount, 1, MaximumCount);
    RefreshControls();
}

void ULxItemSplitWidget::HandleInputChanged(float InValue)
{
    if (!bRefreshing && FMath::IsFinite(InValue))
    {
        // 控件使用浮点数，大堆叠先用双精度夹取，避免浮点舍入超出整数范围。
        const double Clamped = FMath::Clamp(static_cast<double>(InValue), 1.0, static_cast<double>(MaximumCount));
        SetSplitCount(static_cast<int32>(FMath::Clamp<int64>(FMath::RoundToInt64(Clamped), 1, MaximumCount)));
    }
}

void ULxItemSplitWidget::HandleSliderChanged(float InValue)
{
    if (!bRefreshing && FMath::IsFinite(InValue))
    {
        // 按整数上限插值，滑条末端始终保留原堆一个物品。
        const double Ratio = FMath::Clamp(static_cast<double>(InValue), 0.0, 1.0);
        const int64 Count = 1 + FMath::RoundToInt64(Ratio * static_cast<double>(MaximumCount - 1));
        SetSplitCount(static_cast<int32>(FMath::Clamp<int64>(Count, 1, MaximumCount)));
    }
}

void ULxItemSplitWidget::ConfirmSplit()
{
    if (bPending || !Router.IsValid()) return;
    SetPending(true);
    if (!Router->ConfirmSplit(SplitCount)) SetPending(false);
}

void ULxItemSplitWidget::CancelSplit()
{
    if (!bPending && Router.IsValid()) Router->CancelSplit();
}

void ULxItemSplitWidget::ShowFailure(const FText& Reason)
{
    StatusMessage = Reason;
    bPending = false;
    RefreshControls();
}

void ULxItemSplitWidget::SetPending(bool bInPending)
{
    bPending = bInPending;
    if (bPending) StatusMessage = NSLOCTEXT("LxItemSplit", "Waiting", "正在处理……");
    RefreshControls();
}

void ULxItemSplitWidget::RefreshControls()
{
    if (bRefreshing) return;
    bRefreshing = true;
    if (CountInput)
    {
        CountInput->SetMinValue(1.f);
        CountInput->SetMaxValue(static_cast<float>(MaximumCount));
        CountInput->SetMinSliderValue(1.f);
        CountInput->SetMaxSliderValue(static_cast<float>(MaximumCount));
        CountInput->SetDelta(1.f);
        CountInput->SetValue(static_cast<float>(SplitCount));
        CountInput->SetIsEnabled(!bPending);
    }
    if (CountSlider)
    {
        CountSlider->SetValue(MaximumCount > 1 ? static_cast<float>(SplitCount - 1) / (MaximumCount - 1) : 0.f);
        CountSlider->SetIsEnabled(!bPending && MaximumCount > 1);
    }
    if (ConfirmButton) ConfirmButton->SetIsEnabled(!bPending);
    if (CancelButton) CancelButton->SetIsEnabled(!bPending);
    if (StatusText) StatusText->SetText(StatusMessage);
    bRefreshing = false;
    if (!IsDesignTime()) OnSplitDataUpdated(SplitCount, MaximumCount, bPending, StatusMessage);
}
