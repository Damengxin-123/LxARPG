#include "LxProfessionListItemWidget.h"

#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "LxProfessionUIData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"

void ULxProfessionListItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	ProfessionListItemData = Cast<ULxProfessionListItemUIData>(ListItemObject);
	OnProfessionListItemUpdated(ProfessionListItemData);
	if (auto* Name = Cast<URichTextBlock>(GetWidgetFromName(TEXT("职业名称"))))
		Name->SetText(ProfessionListItemData && ProfessionListItemData->ProfessionDefinition
			? ProfessionListItemData->ProfessionDefinition->GetProfessionBaseInfo().ProfessionName.Text : FText::GetEmpty());
	if (auto* Icon = Cast<UImage>(GetWidgetFromName(TEXT("职业图标"))))
		Icon->SetVisibility(ProfessionListItemData && ProfessionListItemData->ProfessionIconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("职业等级"))))
		Text->SetText(FText::FromString(ProfessionListItemData && ProfessionListItemData->bLearned
			? FString::Printf(TEXT("等级 %d / %d"), ProfessionListItemData->CurrentLevel, ProfessionListItemData->MaxLevel) : TEXT("尚未学习")));
}

void ULxProfessionListItemWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	OnProfessionListItemSelectedStateUpdated(ProfessionListItemData, bIsSelected);
}

FReply ULxProfessionListItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ProfessionListItemData != nullptr)
	{
		ProfessionListItemData->RequestSelectProfession();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
