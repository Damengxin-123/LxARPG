#include "LxAICharacterInfoWidget.h"

void ULxAICharacterInfoWidget::UpdateAIBehaviorText(const FString& InBehaviorText)
{
	OnAIBehaviorTextUpdated(InBehaviorText);
}

void ULxAICharacterInfoWidget::UpdateAIHealthPercent(const float InHealthPercent)
{
	OnAIHealthPercentUpdated(FMath::Clamp(InHealthPercent, 0.0f, 1.0f));
}

void ULxAICharacterInfoWidget::ShowAIReceivedDamageText(const FString& InDamageText)
{
	OnAIReceivedDamageText(InDamageText);
}
