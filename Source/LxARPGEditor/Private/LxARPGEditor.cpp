#include "LxARPGEditor.h"

#include "AssetToolsModule.h"
#include "LxQuestSeriesAssetTypeActions.h"
#include "LxInteractionTreeAssetTypeActions.h"
#include "LxAIBehaviorTreeAssetTypeActions.h"
#include "LxSkillFlowAssetTypeActions.h"

void FLxARPGEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	SkillFlowAssetTypeActions = MakeShared<FLxSkillFlowAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(SkillFlowAssetTypeActions.ToSharedRef());
	QuestSeriesAssetTypeActions = MakeShared<FLxQuestSeriesAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(QuestSeriesAssetTypeActions.ToSharedRef());
	InteractionTreeAssetTypeActions = MakeShared<FLxInteractionTreeAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(InteractionTreeAssetTypeActions.ToSharedRef());
	AIBehaviorTreeAssetTypeActions = MakeShared<FLxAIBehaviorTreeAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(AIBehaviorTreeAssetTypeActions.ToSharedRef());
}

void FLxARPGEditorModule::ShutdownModule()
{
	if (SkillFlowAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(SkillFlowAssetTypeActions.ToSharedRef());
	SkillFlowAssetTypeActions.Reset();
	if (AIBehaviorTreeAssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
	{
		FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().UnregisterAssetTypeActions(AIBehaviorTreeAssetTypeActions.ToSharedRef());
	}
	AIBehaviorTreeAssetTypeActions.Reset();
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
