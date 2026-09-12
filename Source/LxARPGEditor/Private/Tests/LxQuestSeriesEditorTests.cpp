#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraphPin.h"
#include "GameplayTagsEditorModule.h"
#include "GameplayTagsManager.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "LxQuestSeriesEdGraph.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxQuestSeriesAssetTest,
	"LxARPG.Quest.QuestSeriesAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxQuestSeriesAssetTest::RunTest(const FString& Parameters)
{
	ULxQuestSeriesAsset* QuestSeriesAsset = NewObject<ULxQuestSeriesAsset>();
	IGameplayTagsEditorModule::Get().AddTransientEditorGameplayTag(TEXT("任务.自动化测试"));
	IGameplayTagsEditorModule::Get().AddTransientEditorGameplayTag(TEXT("任务.自动化测试.第一任务"));
	IGameplayTagsEditorModule::Get().AddTransientEditorGameplayTag(TEXT("任务.自动化测试.第二任务"));
	const FGameplayTag QuestSeriesId = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("任务.自动化测试"));
	const FGameplayTag FirstQuestId = UGameplayTagsManager::Get().RequestGameplayTag(
		TEXT("任务.自动化测试.第一任务"));
	const FGameplayTag SecondQuestId = UGameplayTagsManager::Get().RequestGameplayTag(
		TEXT("任务.自动化测试.第二任务"));
	QuestSeriesAsset->SetQuestSeriesId(QuestSeriesId);
	TestEqual(TEXT("任务系列资产应使用标签作为系列ID"), QuestSeriesAsset->GetQuestSeriesId(), QuestSeriesId);
	TestTrue(TEXT("具体任务ID应为任务系列ID的子标签"), QuestSeriesAsset->IsQuestIdInSeries(FirstQuestId));
	TestFalse(TEXT("任务系列ID本身不能作为具体任务ID"), QuestSeriesAsset->IsQuestIdInSeries(QuestSeriesId));
	TestEqual(TEXT("资产管理器名称应来自任务系列标签"),
		QuestSeriesAsset->GetPrimaryAssetId().PrimaryAssetName, QuestSeriesId.GetTagName());

	ULxQuestSeriesAsset* MigrationAsset = NewObject<ULxQuestSeriesAsset>();
	MigrationAsset->SetQuestSeriesId(QuestSeriesId);
	MigrationAsset->EditQuestNodes().Reserve(2);
	FLxQuestNodeDefinition& LegacyFirstNode = MigrationAsset->EditQuestNodes().AddDefaulted_GetRef();
	LegacyFirstNode.NodeId_DEPRECATED = FGuid::NewGuid();
	LegacyFirstNode.QuestId = FirstQuestId;
	FLxQuestNodeDefinition& LegacySecondNode = MigrationAsset->EditQuestNodes().AddDefaulted_GetRef();
	LegacySecondNode.NodeId_DEPRECATED = FGuid::NewGuid();
	LegacySecondNode.QuestId = FirstQuestId;
	TestTrue(TEXT("旧任务节点ID应在打开编辑器时被修复"), MigrationAsset->RepairEditorData());
	TestTrue(TEXT("迁移后的第一个内部节点ID应有效"), LegacyFirstNode.EditorNodeId.IsValid());
	TestTrue(TEXT("迁移后的第二个内部节点ID应有效"), LegacySecondNode.EditorNodeId.IsValid());
	TestNotEqual(TEXT("迁移后的每个任务节点必须拥有独立内部ID"),
		LegacyFirstNode.EditorNodeId, LegacySecondNode.EditorNodeId);
	TestTrue(TEXT("修复后第一个重复任务ID应保留"), LegacyFirstNode.QuestId.IsValid());
	TestFalse(TEXT("修复后其余重复任务ID应清空并等待重新选择"), LegacySecondNode.QuestId.IsValid());

	FLxQuestNodeDefinition& FirstQuestNode = QuestSeriesAsset->EditQuestNodes().AddDefaulted_GetRef();
	FirstQuestNode.QuestId = FirstQuestId;
	FirstQuestNode.EditorNodeId = FGuid::NewGuid();
	FirstQuestNode.DeveloperName = TEXT("FirstQuest");
	const FGuid FirstQuestEditorNodeId = FirstQuestNode.EditorNodeId;
	FLxQuestNodeDefinition& SecondQuestNode = QuestSeriesAsset->EditQuestNodes().AddDefaulted_GetRef();
	SecondQuestNode.QuestId = SecondQuestId;
	SecondQuestNode.EditorNodeId = FGuid::NewGuid();
	SecondQuestNode.DeveloperName = TEXT("SecondQuest");
	const FGuid SecondQuestEditorNodeId = SecondQuestNode.EditorNodeId;

	ULxQuestSeriesEdGraph* QuestGraph = NewObject<ULxQuestSeriesEdGraph>(QuestSeriesAsset);
	QuestGraph->Schema = ULxQuestSeriesEdGraphSchema::StaticClass();
	QuestSeriesAsset->SetEditorGraph(QuestGraph);

	FGraphNodeCreator<ULxQuestSeriesEdGraphNode> FirstNodeCreator(*QuestGraph);
	ULxQuestSeriesEdGraphNode* FirstGraphNode = FirstNodeCreator.CreateNode();
	FirstGraphNode->SetQuestEditorNodeId(FirstQuestEditorNodeId);
	FirstNodeCreator.Finalize();

	FGraphNodeCreator<ULxQuestSeriesEdGraphNode> SecondNodeCreator(*QuestGraph);
	ULxQuestSeriesEdGraphNode* SecondGraphNode = SecondNodeCreator.CreateNode();
	SecondGraphNode->SetQuestEditorNodeId(SecondQuestEditorNodeId);
	SecondNodeCreator.Finalize();

	UEdGraphPin* OutputPin = FirstGraphNode->FindPin(TEXT("后续"), EGPD_Output);
	UEdGraphPin* InputPin = SecondGraphNode->FindPin(TEXT("前置"), EGPD_Input);
	TestNotNull(TEXT("上级节点应创建后续输出引脚"), OutputPin);
	TestNotNull(TEXT("下级节点应创建前置输入引脚"), InputPin);

	const ULxQuestSeriesEdGraphSchema* GraphSchema = GetDefault<ULxQuestSeriesEdGraphSchema>();
	TestTrue(TEXT("当前系列节点应能从输出连接到输入"), GraphSchema->TryCreateConnection(OutputPin, InputPin));
	QuestGraph->SynchronizeLinksToAsset();

	TestEqual(TEXT("关系图应生成一条运行时任务关系"), QuestSeriesAsset->GetQuestLinks().Num(), 1);
	TestEqual(TEXT("应能查询上级节点的直接下级任务"),
		QuestSeriesAsset->GetDirectSuccessorQuestIds(FirstQuestId).Num(), 1);
	TestEqual(TEXT("应能查询下级节点的直接上级任务"),
		QuestSeriesAsset->GetDirectPredecessorQuestIds(SecondQuestId).Num(), 1);

	FLxQuestNodeDefinition QueriedQuestNode;
	TestTrue(TEXT("应能通过任务ID标签取得静态任务配置"),
		QuestSeriesAsset->GetQuestNode(SecondQuestId, QueriedQuestNode));
	TestEqual(TEXT("查询到的任务节点开发名称应一致"), QueriedQuestNode.DeveloperName, FName(TEXT("SecondQuest")));
	return true;
}

#endif
