#include "LxProfessionLevelNodeWidget.h"

#include "LxProfessionUIData.h"
#include "Components/TextBlock.h"

void ULxProfessionLevelNodeWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	ProfessionLevelNodeData = Cast<ULxProfessionLevelNodeUIData>(ListItemObject);
	OnProfessionLevelNodeUpdated(ProfessionLevelNodeData);
	if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("等级经验文本"))))
	{
		const auto* Data = ProfessionLevelNodeData.Get();
		Text->SetText(FText::FromString(!Data ? TEXT("") : Data->bCurrentLevel
			? (Data->RequiredExperience > 0.f ? FString::Printf(TEXT("当前 · %.0f / %.0f 经验"), Data->CurrentExperience, Data->RequiredExperience) : TEXT("当前 · 已满级"))
			: Data->bUnlocked ? TEXT("已解锁") : TEXT("待解锁")));
	}
}
