#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxStaticDataManagerTest,
	"LxARPG.StaticData.QuestModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxStaticDataManagerTest::RunTest(const FString& Parameters)
{
	const FGameplayTag QuestSeriesId = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("任务.新手任务"));
	const FGameplayTag QuestId = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("任务.新手任务.想离开新手村"));

	ULxQuestSeriesAsset* QuestSeriesAsset = NewObject<ULxQuestSeriesAsset>();
	QuestSeriesAsset->SetQuestSeriesId(QuestSeriesId);
	FLxQuestNodeDefinition& QuestNode = QuestSeriesAsset->EditQuestNodes().AddDefaulted_GetRef();
	QuestNode.QuestId = QuestId;
	QuestNode.DeveloperName = TEXT("LeaveBeginnerVillage");

	UDataTable* QuestSeriesIndexTable = NewObject<UDataTable>();
	QuestSeriesIndexTable->RowStruct = FLxQuestSeriesRegistryRow::StaticStruct();
	FLxQuestSeriesRegistryRow RegistryRow;
	RegistryRow.QuestSeriesId = QuestSeriesId;
	RegistryRow.QuestSeriesAsset = QuestSeriesAsset;
	QuestSeriesIndexTable->AddRow(TEXT("BeginnerQuestSeries"), RegistryRow);

	ULxGameDataTablesManager* LegacyDataTablesManager = NewObject<ULxGameDataTablesManager>();
	LegacyDataTablesManager->m_pQuestSeriesIndexTable = QuestSeriesIndexTable;

	ULxGlobalStaticDataManager* StaticDataManager = NewObject<ULxGlobalStaticDataManager>();
	StaticDataManager->Initialize(LegacyDataTablesManager);
	TestTrue(TEXT("全局静态数据管理器应完成初始化"), StaticDataManager->IsInitialized());

	ULxQuestStaticDataModule* QuestModule = StaticDataManager->GetQuestStaticDataModule();
	TestNotNull(TEXT("全局静态数据管理器应创建任务子模块"), QuestModule);
	if (!QuestModule)
	{
		return false;
	}

	TestTrue(TEXT("任务静态数据模块应读取索引表"), QuestModule->IsInitialized());
	TestTrue(TEXT("任务系列ID应已经登记"), QuestModule->ContainsQuestSeries(QuestSeriesId));
	TestNull(TEXT("任务系列在首次请求前不应被强引用缓存"), QuestModule->FindLoadedQuestSeries(QuestSeriesId));
	TestEqual(TEXT("首次请求应加载索引指向的任务系列资产"),
		QuestModule->LoadQuestSeries(QuestSeriesId), QuestSeriesAsset);

	FLxQuestNodeDefinition LoadedQuestNode;
	TestTrue(TEXT("应能通过系列ID和任务ID查询静态任务配置"),
		QuestModule->GetQuestNode(QuestSeriesId, QuestId, LoadedQuestNode));
	TestEqual(TEXT("查询到的任务开发名称应一致"), LoadedQuestNode.DeveloperName,
		FName(TEXT("LeaveBeginnerVillage")));

	QuestModule->UnloadQuestSeries(QuestSeriesId);
	TestNull(TEXT("卸载后模块不应继续持有任务系列强引用"),
		QuestModule->FindLoadedQuestSeries(QuestSeriesId));
	StaticDataManager->Deinitialize();
	TestFalse(TEXT("反初始化后全局静态数据管理器应回到未初始化状态"), StaticDataManager->IsInitialized());
	return true;
}

#endif
