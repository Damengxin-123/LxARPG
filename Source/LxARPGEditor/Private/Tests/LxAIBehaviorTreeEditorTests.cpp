#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Editor.h"
#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/Class.h"
#include <limits>
#include "Framework/Application/SlateApplication.h"
#include "Toolkits/IToolkitHost.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Widgets/SWindow.h"
#include "LxAIBehaviorTreeAssetFactory.h"
#include "LxAIBehaviorTreeAssetEditor.h"
#include "LxAIBehaviorTreeEdGraph.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"

namespace
{
/** 获取指定方向的首个引脚，测试不依赖图节点的内部引脚排列。 */
UEdGraphPin* FindBehaviorPin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
{
	for (UEdGraphPin* Pin : Node->Pins)
		if (Pin->Direction == Direction) return Pin;
	return nullptr;
}

/** 为序列化测试补齐合法标签；此测试不宣称这些标签能解析成已放置的场景对象。 */
void FillBehaviorTestIds(ULxAIBehaviorTreeAsset* Asset)
{
	for (ULxAIBehaviorTreeNodeData* Node : Asset->Nodes)
	{
		Node->PointId = FGameplayTag::RequestGameplayTag(TEXT("AI.点位"));
		Node->RouteId = FGameplayTag::RequestGameplayTag(TEXT("AI.路线"));
		Node->SkillItemId = FGameplayTag::RequestGameplayTag(TEXT("物品.技能.射线.测试单次射线"));
	}
}
}

/** 通过图菜单、连线、编辑事务和磁盘资产验证配置原型。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIBehaviorTreeEditorTest, "LxARPG.AIBehaviorTree.AssetAndEditor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIBehaviorTreeEditorTest::RunTest(const FString& Parameters)
{
	ULxAIBehaviorTreeAssetFactory* Factory = NewObject<ULxAIBehaviorTreeAssetFactory>();
	ULxAIBehaviorTreeAsset* Asset = Cast<ULxAIBehaviorTreeAsset>(Factory->FactoryCreateNew(
		ULxAIBehaviorTreeAsset::StaticClass(), GetTransientPackage(), NAME_None, RF_Transactional, nullptr, GWarn));
	if (!TestNotNull(TEXT("新建AI控制配置资产"), Asset)) return false;
	ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(Asset->EditorGraph);
	if (!TestNotNull(TEXT("工厂创建专用图"), Graph)) return false;
	Graph->InitializeDefaultTree();
	Graph->EnsureEntryNodes();
	TestEqual(TEXT("重复初始化保留四入口、五状态、五阶段、十行为"), Graph->Nodes.Num(), 24);
	TestEqual(TEXT("资产包含二十四个配置节点"), Asset->Nodes.Num(), 24);
	TestEqual(TEXT("四个固定入口"), Asset->Roots.Num(), 4);
	FText Error;
	TestFalse(TEXT("未指定点位路线技能ID的模板提示待配置"), Asset->ValidateTree(Error));
	FillBehaviorTestIds(Asset);
	TestTrue(TEXT("全部类型的完整模板通过配置校验"), Asset->ValidateTree(Error));
	if (!Error.IsEmpty()) AddInfo(Error.ToString());
	TestTrue(TEXT("默认感知与完整行为图通过总配置校验"), Asset->ValidateConfiguration(Error));
	Asset->Perception.LoseSightRadiusMeters = 1.0f;
	TestFalse(TEXT("行为图有效时总配置仍报告感知错误"), Asset->ValidateConfiguration(Error));
	Asset->Perception = FLxAIPerceptionConfig();

	ULxAIBehaviorTreeEdGraphNode* CalmEntry = nullptr;
	ULxAIBehaviorTreeEdGraphNode* EnemyNearEntry = nullptr;
	ULxAIBehaviorTreeEdGraphNode* AttackedEntry = nullptr;
	ULxAIBehaviorTreeEdGraphNode* IdleState = nullptr;
	ULxAIBehaviorTreeEdGraphNode* CombatState = nullptr;
	ULxAIBehaviorTreeEdGraphNode* IdlePhase = nullptr;
	ULxAIBehaviorTreeEdGraphNode* Wait = nullptr;
	ULxAIBehaviorTreeEdGraphNode* CombatPhase = nullptr;
	ULxAIBehaviorTreeEdGraphNode* CombatAction = nullptr;
	for (UEdGraphNode* RawNode : Graph->Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* Node = CastChecked<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::Entry)
		{
			if (Node->Data->Entry == ELxAIBehaviorEntry::Calm) CalmEntry = Node;
			if (Node->Data->Entry == ELxAIBehaviorEntry::EnemyNear) EnemyNearEntry = Node;
			if (Node->Data->Entry == ELxAIBehaviorEntry::Attacked) AttackedEntry = Node;
			continue;
		}
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::State && Node->Data->State == ELxAIBehaviorState::Idle) IdleState = Node;
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::State && Node->Data->State == ELxAIBehaviorState::Combat) CombatState = Node;
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::Phase && Node->Data->State == ELxAIBehaviorState::Idle) IdlePhase = Node;
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::Phase && Node->Data->State == ELxAIBehaviorState::Combat) CombatPhase = Node;
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::Action && Node->Data->Action == ELxAIBehaviorAction::Wait) Wait = Node;
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::Action && Node->Data->Action == ELxAIBehaviorAction::MeleeSkill) CombatAction = Node;
	}
	if (!CalmEntry || !EnemyNearEntry || !AttackedEntry || !IdleState || !CombatState || !IdlePhase || !CombatPhase || !CombatAction || !Wait)
	{
		AddError(TEXT("默认模板缺少预期节点"));
		return false;
	}
	TestFalse(TEXT("固定事件入口不能删除"), CalmEntry->CanUserDeleteNode());
	TestNull(TEXT("事件入口没有输入"), FindBehaviorPin(CalmEntry, EGPD_Input));
	TestTrue(TEXT("靠近与受击共享同一战斗状态"), CombatState->Data->NodeId.IsValid()
		&& FindBehaviorPin(CombatState, EGPD_Input)->LinkedTo.Num() == 2);
	TestNull(TEXT("行为叶节点没有输出"), FindBehaviorPin(Wait, EGPD_Output));
	const UEdGraphSchema* Schema = Graph->GetSchema();
	UEdGraphPin* StateOutput = FindBehaviorPin(IdleState, EGPD_Output);
	UEdGraphPin* PhaseInput = FindBehaviorPin(IdlePhase, EGPD_Input);
	UEdGraphPin* PhaseOutput = FindBehaviorPin(IdlePhase, EGPD_Output);
	UEdGraphPin* WaitInput = FindBehaviorPin(Wait, EGPD_Input);
	Schema->BreakSinglePinLink(PhaseOutput, WaitInput);
	TestFalse(TEXT("断开的行为被报告为未连接"), Asset->ValidateTree(Error));
	TestEqual(TEXT("拒绝状态直接连接行为"), Schema->CanCreateConnection(StateOutput, WaitInput).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝战斗阶段连接闲置行为"), Schema->CanCreateConnection(FindBehaviorPin(CombatPhase, EGPD_Output), WaitInput).Response, CONNECT_RESPONSE_DISALLOW);
	TestTrue(TEXT("同状态阶段可重新连接行为"), Schema->TryCreateConnection(PhaseOutput, WaitInput));
	TestEqual(TEXT("拒绝重复连线"), Schema->CanCreateConnection(PhaseOutput, WaitInput).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝阶段连接状态形成逆向层级"), Schema->CanCreateConnection(PhaseOutput, FindBehaviorPin(IdleState, EGPD_Input)).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝入口连接阶段的跨层边"), Schema->CanCreateConnection(FindBehaviorPin(CalmEntry, EGPD_Output), PhaseInput).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝入口反向连线"), Schema->CanCreateConnection(FindBehaviorPin(IdleState, EGPD_Output), FindBehaviorPin(CalmEntry, EGPD_Output)).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("战斗行为初始只有一个阶段父项"), FindBehaviorPin(CombatAction, EGPD_Input)->LinkedTo.Num(), 1);
	TestEqual(TEXT("同一状态不能重复接到同一入口"), Schema->CanCreateConnection(
		FindBehaviorPin(EnemyNearEntry, EGPD_Output), FindBehaviorPin(CombatState, EGPD_Input)).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("阶段仍只能归属一个状态"), Schema->CanCreateConnection(
		FindBehaviorPin(CombatState, EGPD_Output), FindBehaviorPin(CombatPhase, EGPD_Input)).Response, CONNECT_RESPONSE_DISALLOW);
	FLxAIBehaviorTreeNewNodeAction AddCombatPhase;
	AddCombatPhase.Kind = ELxAIBehaviorNodeKind::Phase;
	AddCombatPhase.State = ELxAIBehaviorState::Combat;
	ULxAIBehaviorTreeEdGraphNode* SharedActionPhase = Cast<ULxAIBehaviorTreeEdGraphNode>(AddCombatPhase.PerformAction(
		Graph, FindBehaviorPin(CombatState, EGPD_Output), FVector2f(700, 1350), false));
	if (!TestNotNull(TEXT("创建第二个战斗阶段"), SharedActionPhase)) return false;
	TestTrue(TEXT("同一战斗行为允许多个阶段父项"), Schema->TryCreateConnection(
		FindBehaviorPin(SharedActionPhase, EGPD_Output), FindBehaviorPin(CombatAction, EGPD_Input)));
	TestEqual(TEXT("共享行为保留两个阶段父项"), FindBehaviorPin(CombatAction, EGPD_Input)->LinkedTo.Num(), 2);
	TestTrue(TEXT("共享行为被保存到第二阶段子序列"), SharedActionPhase->Data->Children.Contains(CombatAction->Data->NodeId));
	TestTrue(TEXT("共享阶段仍满足完整图校验"), Asset->ValidateTree(Error));
	Schema->BreakNodeLinks(*SharedActionPhase);
	SharedActionPhase->DestroyNode();
	Graph->SynchronizeAsset();

	IdleState->Data->HealthRange.Min = 0.4f;
	IdleState->Data->HealthRange.Max = 0.8f;
	IdlePhase->Data->HealthRange.Min = 0.5f;
	IdlePhase->Data->HealthRange.Max = 0.6f;
	TestTrue(TEXT("阶段可进一步缩小生命区间"), Asset->ValidateTree(Error));
	TestTrue(TEXT("包含区间左边界50%"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.5f));
	TestTrue(TEXT("包含区间右边界60%"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.6f));
	TestFalse(TEXT("低于阶段下界不通过"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.49f));
	TestFalse(TEXT("高于阶段上界不通过"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.61f));
	TestFalse(TEXT("负生命比例不通过"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, -0.1f));
	IdlePhase->Data->HealthRange.Max = 0.9f;
	TestFalse(TEXT("阶段不能扩展父状态区间"), Asset->ValidateTree(Error));
	TestFalse(TEXT("越界阶段不能通过条件查询"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.55f));
	IdlePhase->Data->HealthRange.Max = 0.6f;
	IdlePhase->Data->HealthRange.Min = 0.7f;
	TestFalse(TEXT("反向区间被拒绝"), Asset->ValidateTree(Error));
	IdlePhase->Data->HealthRange.Min = 0.5f;
	Schema->BreakSinglePinLink(StateOutput, PhaseInput);
	TestFalse(TEXT("脱离状态的阶段不通过生命条件"), Asset->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.55f));
	Schema->TryCreateConnection(StateOutput, PhaseInput);

	// 通过实际菜单入口创建阶段，继承父状态区间，再执行删除、撤销、重做。
	FLxAIBehaviorTreeNewNodeAction AddPhase;
	AddPhase.Kind = ELxAIBehaviorNodeKind::Phase;
	AddPhase.State = ELxAIBehaviorState::Idle;
	ULxAIBehaviorTreeEdGraphNode* ExtraPhase = Cast<ULxAIBehaviorTreeEdGraphNode>(AddPhase.PerformAction(Graph, StateOutput, FVector2f(650, -300), false));
	if (!TestNotNull(TEXT("拖出引脚创建新阶段"), ExtraPhase)) return false;
	TestEqual(TEXT("新阶段继承父状态下限"), ExtraPhase->Data->HealthRange.Min, 0.4f);
	TestEqual(TEXT("新阶段继承父状态上限"), ExtraPhase->Data->HealthRange.Max, 0.8f);
	const FGuid ExtraId = ExtraPhase->Data->NodeId;
	{
		const FScopedTransaction Transaction(NSLOCTEXT("AI行为树测试", "删除阶段", "删除测试阶段"));
		Graph->Modify();
		Asset->Modify();
		ExtraPhase->Modify();
		Schema->BreakNodeLinks(*ExtraPhase);
		ExtraPhase->DestroyNode();
		Graph->SynchronizeAsset();
	}
	TestNull(TEXT("删除会移除派生配置"), Asset->FindNode(ExtraId));
	GEditor->UndoTransaction();
	Graph->SynchronizeAsset();
	TestNotNull(TEXT("撤销恢复阶段及配置"), Asset->FindNode(ExtraId));
	GEditor->RedoTransaction();
	Graph->SynchronizeAsset();
	TestNull(TEXT("重做再次移除阶段"), Asset->FindNode(ExtraId));
	TestTrue(TEXT("删除恢复原有有效模板"), Asset->ValidateTree(Error));
	const FIntPoint OriginalPosition(IdleState->NodePosX, IdleState->NodePosY);
	{
		const FScopedTransaction Transaction(NSLOCTEXT("AI控制配置测试", "移动节点", "移动闲置状态节点"));
		IdleState->Modify();
		IdleState->NodePosX += 30;
		IdleState->NodePosY += 20;
	}
	const FIntPoint MovedPosition(IdleState->NodePosX, IdleState->NodePosY);
	GEditor->UndoTransaction();
	TestTrue(TEXT("撤销恢复节点位置"), FIntPoint(IdleState->NodePosX, IdleState->NodePosY) == OriginalPosition);
	GEditor->RedoTransaction();
	TestTrue(TEXT("重做恢复节点位置"), FIntPoint(IdleState->NodePosX, IdleState->NodePosY) == MovedPosition);
	TestTrue(TEXT("移动节点不改变显式状态顺序"), Asset->Roots.Num() == 4 && CalmEntry->Data->Children.Contains(IdleState->Data->NodeId));
	const int32 OriginalPriority = CalmEntry->Data->EntryPriority;
	{
		const FScopedTransaction Transaction(NSLOCTEXT("AI控制配置测试", "修改入口优先级", "修改平静入口优先级"));
		CalmEntry->Data->Modify();
		CalmEntry->Data->EntryPriority = 25;
	}
	GEditor->UndoTransaction();
	TestEqual(TEXT("撤销入口优先级"), CalmEntry->Data->EntryPriority, OriginalPriority);
	GEditor->RedoTransaction();
	TestEqual(TEXT("重做入口优先级"), CalmEntry->Data->EntryPriority, 25);

	// 使用非默认值验证类型级感知配置的撤销、独立复制和磁盘往返。
	const FLxAIPerceptionConfig DefaultPerception = Asset->Perception;
	{
		const FScopedTransaction Transaction(NSLOCTEXT("AI控制配置测试", "修改感知", "修改类型感知配置"));
		Asset->Modify();
		Asset->Perception.bEnableSight = false;
		Asset->Perception.SightRadiusMeters = 42.0f;
		Asset->Perception.LoseSightRadiusMeters = 48.0f;
		Asset->Perception.SightHalfAngleDegrees = 75.0f;
		Asset->Perception.SightMemorySeconds = 8.0f;
		Asset->Perception.bEnableHearing = false;
		Asset->Perception.HearingRadiusMeters = 26.0f;
		Asset->Perception.HearingMemorySeconds = 6.0f;
		Asset->Perception.bDetectEnemies = false;
		Asset->Perception.bDetectNeutrals = true;
		Asset->Perception.bDetectFriendlies = true;
	}
	const FLxAIPerceptionConfig CustomPerception = Asset->Perception;
	GEditor->UndoTransaction();
	TestTrue(TEXT("撤销恢复整份感知参数"), FLxAIPerceptionConfig::StaticStruct()->CompareScriptStruct(&Asset->Perception, &DefaultPerception, 0));
	GEditor->RedoTransaction();
	TestTrue(TEXT("重做恢复整份类型感知配置"), FLxAIPerceptionConfig::StaticStruct()->CompareScriptStruct(&Asset->Perception, &CustomPerception, 0));
	const FLxAIAnalysisConfig DefaultAnalysis = Asset->Analysis;
	{
		const FScopedTransaction Transaction(NSLOCTEXT("AI控制配置测试", "修改分析", "修改类型分析配置"));
		Asset->Modify();
		Asset->Analysis.NearEnterDistanceMeters = 4.0f;
		Asset->Analysis.NearExitDistanceMeters = 6.0f;
		Asset->Analysis.AttackedAlertSeconds = 9.0f;
	}
	const FLxAIAnalysisConfig CustomAnalysis = Asset->Analysis;
	GEditor->UndoTransaction();
	TestTrue(TEXT("撤销恢复整份分析参数"), FLxAIAnalysisConfig::StaticStruct()->CompareScriptStruct(&Asset->Analysis, &DefaultAnalysis, 0));
	GEditor->RedoTransaction();
	TestTrue(TEXT("重做恢复整份分析参数"), FLxAIAnalysisConfig::StaticStruct()->CompareScriptStruct(&Asset->Analysis, &CustomAnalysis, 0));
	TestTrue(TEXT("自定义分析参数有效"), Asset->ValidateConfiguration(Error));
	ULxAIBehaviorTreeAsset* Copy = DuplicateObject<ULxAIBehaviorTreeAsset>(Asset, GetTransientPackage());
	TestTrue(TEXT("复制资产保留全部感知参数"), FLxAIPerceptionConfig::StaticStruct()->CompareScriptStruct(&Copy->Perception, &CustomPerception, 0));
	TestTrue(TEXT("复制资产保留分析参数"), FLxAIAnalysisConfig::StaticStruct()->CompareScriptStruct(&Copy->Analysis, &CustomAnalysis, 0));
	Copy->Perception.SightRadiusMeters = 10.0f;
	TestEqual(TEXT("修改另一个角色类型配置不会改动原资产"), Asset->Perception.SightRadiusMeters, 42.0f);
	TestNotEqual(TEXT("复制资产不共享节点配置"), Copy->Nodes[0].Get(), Asset->Nodes[0].Get());
	CastChecked<ULxAIBehaviorTreeEdGraph>(Copy->EditorGraph)->SynchronizeAsset();
	TestTrue(TEXT("复制的图仍引用自己的节点配置"), Copy->ValidateTree(Error));
	for (const ULxAIBehaviorTreeNodeData* Node : Copy->Nodes)
		TestTrue(TEXT("复制节点归属于新资产"), Node->GetOuter() == Copy);

	// 从独立磁盘文件重载到另一包，避免把仍在内存中的原资产误认为保存验证。
	const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	UPackage* Package = CreatePackage(*(TEXT("/Temp/AI行为树保存_") + Suffix));
	ULxAIBehaviorTreeAsset* SavedAsset = DuplicateObject<ULxAIBehaviorTreeAsset>(Asset, Package, TEXT("行为树保存测试"));
	SavedAsset->SetFlags(RF_Public | RF_Standalone);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const FString TestDirectory = FPaths::ProjectSavedDir() / TEXT("Tests");
	IFileManager::Get().MakeDirectory(*TestDirectory, true);
	const FString File = TestDirectory / TEXT("AI行为树保存测试.uasset");
	if (!TestTrue(TEXT("图配置和布局保存到磁盘"), UPackage::SavePackage(Package, SavedAsset, *File, SaveArgs))) return false;
	UPackage* LoadedPackage = LoadPackage(CreatePackage(*(TEXT("/Temp/AI行为树重载_") + Suffix)), *File, LOAD_None);
	ULxAIBehaviorTreeAsset* Loaded = LoadedPackage ? FindObject<ULxAIBehaviorTreeAsset>(LoadedPackage, TEXT("行为树保存测试")) : nullptr;
	if (TestNotNull(TEXT("从磁盘独立重载AI行为树"), Loaded))
	{
		TestTrue(TEXT("重载后生命条件、标签和层级有效"), Loaded->ValidateTree(Error));
		TestEqual(TEXT("重载后节点数量一致"), Loaded->Nodes.Num(), Asset->Nodes.Num());
		TestNotNull(TEXT("重载后保留可继续编辑的图"), Loaded->EditorGraph.Get());
		TestTrue(TEXT("重载后生命值仍匹配"), Loaded->IsPhaseHealthEligible(IdlePhase->Data->NodeId, 0.55f));
		bool bFoundMovedState = false;
		for (UEdGraphNode* RawNode : CastChecked<ULxAIBehaviorTreeEdGraph>(Loaded->EditorGraph)->Nodes)
		{
			const ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
			if (Node && Node->Data && Node->Data->NodeId == IdleState->Data->NodeId)
			{
				bFoundMovedState = FIntPoint(Node->NodePosX, Node->NodePosY) == MovedPosition;
				break;
			}
		}
		TestTrue(TEXT("新图节点位置保存后仍保留"), bFoundMovedState);
		TestTrue(TEXT("感知开关、距离、角度、记忆和目标筛选均从磁盘恢复"), FLxAIPerceptionConfig::StaticStruct()->CompareScriptStruct(&Loaded->Perception, &CustomPerception, 0));
		TestTrue(TEXT("分析参数从磁盘恢复"), FLxAIAnalysisConfig::StaticStruct()->CompareScriptStruct(&Loaded->Analysis, &CustomAnalysis, 0));
		TestEqual(TEXT("磁盘保留四入口"), Loaded->Roots.Num(), 4);
		TestTrue(TEXT("重载后的总控制配置有效"), Loaded->ValidateConfiguration(Error));
	}
	// 构造并保存真实旧格式包：保留原状态、阶段、行为图，只用旧开始节点连接状态。
	UPackage* LegacyPackage = CreatePackage(*(TEXT("/Temp/AI旧图保存_") + Suffix));
	ULxAIBehaviorTreeAsset* LegacyAsset = DuplicateObject<ULxAIBehaviorTreeAsset>(Asset, LegacyPackage, TEXT("旧行为树迁移测试"));
	LegacyAsset->SetFlags(RF_Public | RF_Standalone);
	ULxAIBehaviorTreeEdGraph* LegacyGraph = CastChecked<ULxAIBehaviorTreeEdGraph>(LegacyAsset->EditorGraph);
	TArray<ULxAIBehaviorTreeEdGraphNode*> LegacyEntries;
	TArray<ULxAIBehaviorTreeEdGraphNode*> LegacyStates;
	TMap<FGuid, FIntPoint> LegacyPositions;
	for (UEdGraphNode* RawNode : LegacyGraph->Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* Node = CastChecked<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (Node->Data->Kind == ELxAIBehaviorNodeKind::Entry) LegacyEntries.Add(Node);
		else
		{
			LegacyPositions.Add(Node->Data->NodeId, FIntPoint(Node->NodePosX, Node->NodePosY));
			if (Node->Data->Kind == ELxAIBehaviorNodeKind::State) LegacyStates.Add(Node);
		}
	}
	for (ULxAIBehaviorTreeEdGraphNode* EntryNode : LegacyEntries) EntryNode->DestroyNode();
	LegacyAsset->Nodes.RemoveAll([](const TObjectPtr<ULxAIBehaviorTreeNodeData>& Data)
	{
		return Data && Data->Kind == ELxAIBehaviorNodeKind::Entry;
	});
	LegacyAsset->Roots.Reset();
	FGraphNodeCreator<ULxAIBehaviorTreeEdGraphNode> OldStartCreator(*LegacyGraph);
	ULxAIBehaviorTreeEdGraphNode* OldStart = OldStartCreator.CreateNode();
	OldStart->bStart = true;
	OldStart->NodePosX = 0;
	OldStart->NodePosY = 810;
	OldStartCreator.Finalize();
	for (ULxAIBehaviorTreeEdGraphNode* StateNode : LegacyStates)
	{
		LegacyAsset->Roots.Add(StateNode->Data->NodeId);
		FindBehaviorPin(OldStart, EGPD_Output)->MakeLinkTo(FindBehaviorPin(StateNode, EGPD_Input));
	}
	const FString LegacyFile = TestDirectory / TEXT("AI旧图迁移测试.uasset");
	if (!TestTrue(TEXT("真实旧开始节点图保存到磁盘"), UPackage::SavePackage(LegacyPackage, LegacyAsset, *LegacyFile, SaveArgs))) return false;
	UPackage* MigratedPackage = LoadPackage(CreatePackage(*(TEXT("/Temp/AI旧图重载_") + Suffix)), *LegacyFile, LOAD_None);
	ULxAIBehaviorTreeAsset* Migrated = MigratedPackage ? FindObject<ULxAIBehaviorTreeAsset>(MigratedPackage, TEXT("旧行为树迁移测试")) : nullptr;
	if (TestNotNull(TEXT("从磁盘加载真实旧格式包"), Migrated))
	{
		ULxAIBehaviorTreeEdGraph* MigratedGraph = CastChecked<ULxAIBehaviorTreeEdGraph>(Migrated->EditorGraph);
		MigratedGraph->EnsureEntryNodes();
		TestEqual(TEXT("旧图升级为四个事件入口"), Migrated->Roots.Num(), 4);
		TestEqual(TEXT("旧图升级后保留原二十节点并新增四入口"), Migrated->Nodes.Num(), 24);
		TestTrue(TEXT("旧图升级后分支配置仍有效"), Migrated->ValidateTree(Error));
		for (UEdGraphNode* RawNode : MigratedGraph->Nodes)
		{
			ULxAIBehaviorTreeEdGraphNode* Node = CastChecked<ULxAIBehaviorTreeEdGraphNode>(RawNode);
			TestFalse(TEXT("旧开始节点已移除"), Node->bStart);
			if (Node->Data && Node->Data->Kind != ELxAIBehaviorNodeKind::Entry)
			{
				const FIntPoint* Original = LegacyPositions.Find(Node->Data->NodeId);
				TestTrue(TEXT("旧配置节点保留原GUID"), Original != nullptr);
				if (Original) TestTrue(TEXT("旧配置节点保留原画布位置"), FIntPoint(Node->NodePosX, Node->NodePosY) == *Original);
			}
		}
		const FString MigratedFile = TestDirectory / TEXT("AI旧图迁移后保存测试.uasset");
		if (TestTrue(TEXT("迁移后的四入口图可以再次保存"), UPackage::SavePackage(MigratedPackage, Migrated, *MigratedFile, SaveArgs)))
		{
			UPackage* ReopenedPackage = LoadPackage(CreatePackage(*(TEXT("/Temp/AI旧图再重载_") + Suffix)), *MigratedFile, LOAD_None);
			ULxAIBehaviorTreeAsset* Reopened = ReopenedPackage ? FindObject<ULxAIBehaviorTreeAsset>(ReopenedPackage, TEXT("旧行为树迁移测试")) : nullptr;
			if (TestNotNull(TEXT("迁移图保存后独立重载"), Reopened))
			{
				TestEqual(TEXT("再次重载仍是四个入口"), Reopened->Roots.Num(), 4);
				TestTrue(TEXT("再次重载的迁移图有效"), Reopened->ValidateTree(Error));
			}
		}
	}
	// 原型阶段创建的旧资产没有感知字段；加载时应取得有效默认值并保留节点。
	const FString ExistingExampleFile = FPaths::ProjectContentDir() / TEXT("AI/行为树示例.uasset");
	if (IFileManager::Get().FileExists(*ExistingExampleFile))
	{
		ULxAIBehaviorTreeAsset* ExistingExample = LoadObject<ULxAIBehaviorTreeAsset>(nullptr, TEXT("/Game/AI/行为树示例.行为树示例"));
		if (TestNotNull(TEXT("旧行为树示例仍可用原路径加载"), ExistingExample))
		{
			TestFalse(TEXT("旧资产仍保留行为节点"), ExistingExample->Nodes.IsEmpty());
			TestTrue(TEXT("旧资产的感知配置有效"), ExistingExample->Perception.ValidateConfiguration(Error));
		}
	}
	else
	{
		AddInfo(TEXT("旧示例文件不存在，跳过旧文件加载检查；本次仍验证新配置的磁盘保存重载。"));
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("CreateAIBehaviorTreeExample")))
	{
		const FString ExampleFile = FPaths::ProjectContentDir() / TEXT("AI/行为树示例.uasset");
		if (!IFileManager::Get().FileExists(*ExampleFile))
		{
			UPackage* ExamplePackage = CreatePackage(TEXT("/Game/AI/行为树示例"));
			// 使用工厂原始模板，避免将测试标签和测试生命值误当成可运行示例配置。
			ULxAIBehaviorTreeAsset* Example = CastChecked<ULxAIBehaviorTreeAsset>(Factory->FactoryCreateNew(
				ULxAIBehaviorTreeAsset::StaticClass(), ExamplePackage, TEXT("行为树示例"), RF_Public | RF_Standalone, nullptr, GWarn));
			IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("AI")), true);
			TestTrue(TEXT("创建供用户评估的中文节点示例"), UPackage::SavePackage(ExamplePackage, Example, *ExampleFile, SaveArgs));
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("SmokeAIBehaviorTreeEditor")) && FApp::CanEverRender())
	{
		Asset->Perception = FLxAIPerceptionConfig();
		CombatPhase->Data->Label = FText::FromString(TEXT("半血阶段"));
		CombatPhase->Data->HealthRange.Min = 0.5f;
		CombatPhase->Data->HealthRange.Max = 0.6f;
		TSharedRef<FLxAIBehaviorTreeAssetEditor> Editor = MakeShared<FLxAIBehaviorTreeAssetEditor>();
		Editor->Init(EToolkitMode::Standalone, nullptr, Asset);
		if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(Editor->GetToolkitHost()->GetParentWidget()))
			Window->Resize(FVector2D(1920, 1080));
		// 等窗口布局刷新后再按实际画布大小缩放，避免使用初始小窗口的几何量。
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Editor]()
		{
			Editor->FocusAnalysisTab();
			Editor->FocusState(ELxAIBehaviorState::Combat);
		}, 0.5f));
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Editor, TestDirectory]()
		{
			TArray<FColor> Pixels;
			FIntVector Size;
			if (TestTrue(TEXT("从真实Slate编辑器捕获界面"), FSlateApplication::Get().TakeScreenshot(Editor->GetToolkitHost()->GetParentWidget(), Pixels, Size)))
			{
				TArray64<uint8> Png;
				const FImageView Image(Pixels.GetData(), Size.X, Size.Y, ERawImageFormat::BGRA8);
				if (TestTrue(TEXT("将编辑器截图编码为PNG"), FImageUtils::CompressImage(Png, TEXT("png"), Image)))
					TestTrue(TEXT("保存编辑器视觉证据"), FFileHelper::SaveArrayToFile(Png, *(TestDirectory / TEXT("AI控制配置编辑器.png"))));
			}
			Editor->FocusEntries();
		}, 1.0f));
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Editor, TestDirectory]()
		{
			TArray<FColor> Pixels;
			FIntVector Size;
			if (TestTrue(TEXT("捕获四个分析入口与分析参数面板"), FSlateApplication::Get().TakeScreenshot(Editor->GetToolkitHost()->GetParentWidget(), Pixels, Size)))
			{
				TArray64<uint8> Png;
				const FImageView Image(Pixels.GetData(), Size.X, Size.Y, ERawImageFormat::BGRA8);
				if (TestTrue(TEXT("四入口截图编码为PNG"), FImageUtils::CompressImage(Png, TEXT("png"), Image)))
					TestTrue(TEXT("保存四入口视觉证据"), FFileHelper::SaveArrayToFile(Png, *(TestDirectory / TEXT("AI控制配置编辑器-四入口.png"))));
			}
			Editor->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed);
		}, 0.7f));
	}
	return true;
}

/** 感知配置的有效边界、关闭能力和非有限参数验证。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIControlPerceptionTest, "LxARPG.AIControlConfig.Perception",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIControlPerceptionTest::RunTest(const FString& Parameters)
{
	FLxAIPerceptionConfig Config;
	FText Error;
	TestTrue(TEXT("默认感知参数有效"), Config.ValidateConfiguration(Error));
	Config.LoseSightRadiusMeters = Config.SightRadiusMeters;
	Config.SightHalfAngleDegrees = 180.0f;
	Config.SightMemorySeconds = 0.0f;
	Config.HearingMemorySeconds = 0.0f;
	Config.bDetectEnemies = false;
	TestTrue(TEXT("等距、全向视野、无限记忆及不识别任何阵营均为合法配置"), Config.ValidateConfiguration(Error));
	Config.SightHalfAngleDegrees = 181.0f;
	TestFalse(TEXT("拒绝大于180度的半角"), Config.ValidateConfiguration(Error));
	Config.SightHalfAngleDegrees = -1.0f;
	TestFalse(TEXT("拒绝负半角"), Config.ValidateConfiguration(Error));
	Config.SightHalfAngleDegrees = 0.0f;
	TestTrue(TEXT("允许零半角边界"), Config.ValidateConfiguration(Error));
	Config.LoseSightRadiusMeters = Config.SightRadiusMeters - 1.0f;
	TestFalse(TEXT("拒绝丢失距离小于发现距离"), Config.ValidateConfiguration(Error));
	Config = FLxAIPerceptionConfig();
	Config.SightRadiusMeters = 0.0f;
	TestFalse(TEXT("启用视觉时发现距离必须为正"), Config.ValidateConfiguration(Error));
	Config = FLxAIPerceptionConfig();
	Config.SightMemorySeconds = -1.0f;
	TestFalse(TEXT("拒绝负视觉记忆时间"), Config.ValidateConfiguration(Error));
	Config = FLxAIPerceptionConfig();
	Config.SightRadiusMeters = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("拒绝视觉非数值参数"), Config.ValidateConfiguration(Error));
	Config.bEnableSight = false;
	TestTrue(TEXT("关闭视觉后保留的参数不参与有效性判定"), Config.ValidateConfiguration(Error));
	Config.HearingRadiusMeters = 0.0f;
	TestFalse(TEXT("启用听觉时距离必须为正"), Config.ValidateConfiguration(Error));
	Config.HearingRadiusMeters = 10.0f;
	Config.HearingMemorySeconds = -1.0f;
	TestFalse(TEXT("拒绝负声音记忆时间"), Config.ValidateConfiguration(Error));
	Config.HearingMemorySeconds = std::numeric_limits<float>::infinity();
	TestFalse(TEXT("拒绝听觉无限数值参数"), Config.ValidateConfiguration(Error));
	Config.bEnableHearing = false;
	TestTrue(TEXT("允许同时关闭视觉与听觉"), Config.ValidateConfiguration(Error));
	TestTrue(TEXT("成功校验清除之前的错误"), Error.IsEmpty());
	return true;
}

/** 分析距离滞回和受击警觉时间的边界验证。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIControlAnalysisTest, "LxARPG.AIControlConfig.Analysis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIControlAnalysisTest::RunTest(const FString& Parameters)
{
	FLxAIAnalysisConfig Config;
	FText Error;
	TestTrue(TEXT("默认分析参数有效"), Config.ValidateConfiguration(Error));
	Config.NearExitDistanceMeters = Config.NearEnterDistanceMeters;
	TestFalse(TEXT("靠近退出距离须大于进入距离"), Config.ValidateConfiguration(Error));
	Config.NearExitDistanceMeters = Config.NearEnterDistanceMeters + 0.01f;
	TestTrue(TEXT("严格大于进入距离可用"), Config.ValidateConfiguration(Error));
	Config.AttackedAlertSeconds = -1.0f;
	TestFalse(TEXT("拒绝负警觉时间"), Config.ValidateConfiguration(Error));
	Config.AttackedAlertSeconds = std::numeric_limits<float>::infinity();
	TestFalse(TEXT("拒绝非有限警觉时间"), Config.ValidateConfiguration(Error));
	Config.AttackedAlertSeconds = 0.0f;
	TestTrue(TEXT("允许零警觉时间"), Config.ValidateConfiguration(Error));
	return true;
}

#endif
