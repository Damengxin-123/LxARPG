#include "LxQuestSeriesAssetTypeActions.h"

#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "LxQuestSeriesAssetEditor.h"

#define LOCTEXT_NAMESPACE "LxQuestSeriesAssetTypeActions"

FText FLxQuestSeriesAssetTypeActions::GetName() const
{
	return LOCTEXT("QuestSeriesAssetName", "任务系列");
}

FColor FLxQuestSeriesAssetTypeActions::GetTypeColor() const
{
	return FColor(30, 125, 205);
}

UClass* FLxQuestSeriesAssetTypeActions::GetSupportedClass() const
{
	return ULxQuestSeriesAsset::StaticClass();
}

uint32 FLxQuestSeriesAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

void FLxQuestSeriesAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (ULxQuestSeriesAsset* QuestSeriesAsset = Cast<ULxQuestSeriesAsset>(Object))
		{
			FLxQuestSeriesAssetEditor::CreateEditor(Mode, EditWithinLevelEditor, QuestSeriesAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
