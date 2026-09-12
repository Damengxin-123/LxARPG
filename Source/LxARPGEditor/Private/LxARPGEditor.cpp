#include "LxARPGEditor.h"

#include "AssetToolsModule.h"
#include "LxQuestSeriesAssetTypeActions.h"

void FLxARPGEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	QuestSeriesAssetTypeActions = MakeShared<FLxQuestSeriesAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
}

void FLxARPGEditorModule::ShutdownModule()
{
	if (QuestSeriesAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().UnregisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
	}

	QuestSeriesAssetTypeActions.Reset();
}

IMPLEMENT_MODULE(FLxARPGEditorModule, LxARPGEditor)
