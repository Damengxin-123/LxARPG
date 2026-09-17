#include "LxARPGEditor.h"

#include "AssetToolsModule.h"
#include "LxQuestSeriesAssetTypeActions.h"
#include "LxInteractionTreeAssetTypeActions.h"

void FLxARPGEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	QuestSeriesAssetTypeActions = MakeShared<FLxQuestSeriesAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
	InteractionTreeAssetTypeActions = MakeShared<FLxInteractionTreeAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(InteractionTreeAssetTypeActions.ToSharedRef());
}

void FLxARPGEditorModule::ShutdownModule()
{
	if (InteractionTreeAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(InteractionTreeAssetTypeActions.ToSharedRef());
	}
	InteractionTreeAssetTypeActions.Reset();
	if (QuestSeriesAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().UnregisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
	}

	QuestSeriesAssetTypeActions.Reset();
}

IMPLEMENT_MODULE(FLxARPGEditorModule, LxARPGEditor)
