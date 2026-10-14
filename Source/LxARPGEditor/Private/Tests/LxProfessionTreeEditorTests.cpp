#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Tests/AutomationCommon.h"
#include "LxProfessionTreeAssetFactory.h"
#include "LxProfessionTreeAssetEditor.h"
#include "LxProfessionTreeEdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "HAL/FileManager.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Toolkits/IToolkitHost.h"
#include "Widgets/SWindow.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "SGraphNode.h"
#include "Widgets/SBoxPanel.h"

namespace
{
	/** 为示例添加一个职业，沿用现有职业ID和等级词条配置结构。 */
	ULxProfessionTreeDefinition* AddExampleProfession(ULxProfessionTreeAsset* Asset, const TCHAR* Tag, const TCHAR* Name,
		const TCHAR* Description, ELxProfessionTreeCategory Category, const TCHAR* EntryTag)
	{
		auto* Definition = NewObject<ULxProfessionTreeDefinition>(Asset, NAME_None, RF_Transactional);
		Definition->ProfessionID = FGameplayTag::RequestGameplayTag(Tag);
		Definition->DisplayName = FText::FromString(Name); Definition->Description = FText::FromString(Description);
		Definition->ProfessionCategory = Category;
		for (int32 Level = 1; Level <= 5; ++Level)
		{
			FLxProfessionLevelEffectConfig& Config = Definition->Levels.AddDefaulted_GetRef();
			FLxEntryQuote& Quote = Config.LevelEntryQuotes.AddDefaulted_GetRef();
			Quote.EntryID = FGameplayTag::RequestGameplayTag(EntryTag); Quote.EntryProportion = static_cast<float>(Level);
		}
		Definition->RefreshLevelExperience();
		Asset->Professions.Add(Definition); return Definition;
	}

	/** 构建六职业与四条关系的中文演示数据。 */
	ULxProfessionTreeAsset* MakeExample()
	{
		auto* Factory = NewObject<ULxProfessionTreeAssetFactory>();
		auto* Asset = CastChecked<ULxProfessionTreeAsset>(Factory->FactoryCreateNew(ULxProfessionTreeAsset::StaticClass(),
			GetTransientPackage(), NAME_None, RF_Transactional, nullptr, GWarn));
		AddExampleProfession(Asset, TEXT("职业.战斗.战士"), TEXT("战士"), TEXT("近战职业，展示铁匠增益与魔法师抑制。"), ELxProfessionTreeCategory::Combat, TEXT("词条.属性修改.标量.力量增加1"));
		AddExampleProfession(Asset, TEXT("职业.战斗.魔法师"), TEXT("魔法师"), TEXT("与战士相互抑制，仅保留1%职业加成。"), ELxProfessionTreeCategory::Combat, TEXT("词条.资源修改.上限.生命上限增加1"));
		AddExampleProfession(Asset, TEXT("职业.采集.矿工"), TEXT("矿工"), TEXT("达到3级后满足铁匠的职业前置。"), ELxProfessionTreeCategory::Gathering, TEXT("词条.属性修改.标量.力量增加1"));
		AddExampleProfession(Asset, TEXT("职业.采集.伐木工"), TEXT("伐木工"), TEXT("为矿工提供10%的单向职业增益。"), ELxProfessionTreeCategory::Gathering, TEXT("词条.属性修改.标量.力量增加1"));
		AddExampleProfession(Asset, TEXT("职业.制作.铁匠"), TEXT("铁匠"), TEXT("前置为矿工3级，与战士相互增益20%。"), ELxProfessionTreeCategory::Production, TEXT("词条.属性修改.标量.力量增加1"));
		AddExampleProfession(Asset, TEXT("职业.制作.药剂师"), TEXT("药剂师"), TEXT("无职业关系示例，可自由添加关联。"), ELxProfessionTreeCategory::Production, TEXT("词条.资源修改.上限.生命上限增加1"));
		auto* Graph = CastChecked<ULxProfessionTreeEdGraph>(Asset->EditorGraph);
		const FVector2f Locations[] = { {780, 80}, {780, 380}, {0, 80}, {0, 380}, {390, 80}, {390, 380} };
		TArray<ULxProfessionTreeEdGraphNode*> Nodes;
		for (int32 Index = 0; Index < Asset->Professions.Num(); ++Index)
		{
			FLxProfessionTreePlaceAction Action; Action.Definition = Asset->Professions[Index];
			Nodes.Add(CastChecked<ULxProfessionTreeEdGraphNode>(Action.PerformAction(Graph, nullptr, Locations[Index], false)));
		}
		const auto* Schema = GetDefault<ULxProfessionTreeEdGraphSchema>();
		Schema->TryCreateConnection(Nodes[2]->Pins[2], Nodes[4]->Pins[0]);
		Schema->TryCreateConnection(Nodes[4]->Pins[2], Nodes[0]->Pins[1]);
		Schema->TryCreateConnection(Nodes[1]->Pins[2], Nodes[0]->Pins[1]);
		Schema->TryCreateConnection(Nodes[3]->Pins[2], Nodes[2]->Pins[1]);
		for (ULxProfessionTreeInfluence* Relation : Asset->Influences)
		{
			if (Relation->Kind == ELxProfessionTreeRelation::Prerequisite) Relation->RequiredLevel = 3;
			else if (Relation->Source == Asset->Professions[4]) { Relation->Multiplier = 1.2f; Relation->bMutual = true; }
			else if (Relation->Source == Asset->Professions[1]) { Relation->Multiplier = 0.01f; Relation->bMutual = true; }
			else Relation->Multiplier = 1.1f;
		}
		return Asset;
	}
}

/** 验证关系语义、编辑事务、复制和磁盘重载，并可生成实际演示资产及截图。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfessionTreeEditorTest, "LxARPG.ProfessionTree.AssetAndEditor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 从真实引脚连线验证静态配置和编辑器行为。 */
bool FLxProfessionTreeEditorTest::RunTest(const FString& Parameters)
{
	ULxProfessionTreeAsset* Asset = MakeExample();
	auto* Graph = CastChecked<ULxProfessionTreeEdGraph>(Asset->EditorGraph);
	const auto* Schema = GetDefault<ULxProfessionTreeEdGraphSchema>();
	auto* Warrior = CastChecked<ULxProfessionTreeEdGraphNode>(Graph->Nodes[0]);
	auto* Mage = CastChecked<ULxProfessionTreeEdGraphNode>(Graph->Nodes[1]);
	auto* Miner = CastChecked<ULxProfessionTreeEdGraphNode>(Graph->Nodes[2]);
	auto* Smith = CastChecked<ULxProfessionTreeEdGraphNode>(Graph->Nodes[4]);
	TestEqual(TEXT("示例包含三个分类共六个职业"), Asset->Professions.Num(), 6);
	TestEqual(TEXT("每个节点有两个输入和一个输出"), Warrior->Pins.Num(), 3);
	TestEqual(TEXT("前置只输入"), Warrior->Pins[0]->Direction, EGPD_Input);
	TestEqual(TEXT("增益只输入"), Warrior->Pins[1]->Direction, EGPD_Input);
	TestEqual(TEXT("影响只输出"), Warrior->Pins[2]->Direction, EGPD_Output);
	TestEqual(TEXT("示例关系完整"), Asset->Influences.Num(), 4);
	TestTrue(TEXT("示例通过配置检查"), Asset->ValidateConfiguration().IsEmpty());
	TestEqual(TEXT("铁匠收到前置及反向增益"), Asset->GetIncomingInfluences(Smith->Definition).Num(), 2);
	TestEqual(TEXT("魔法师收到双向抑制"), Asset->GetIncomingInfluences(Mage->Definition).Num(), 1);
	TestEqual(TEXT("拒绝两个输入相连"), Schema->CanCreateConnection(Miner->Pins[0], Smith->Pins[1]).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝自连"), Schema->CanCreateConnection(Warrior->Pins[2], Warrior->Pins[1]).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝前置环"), Schema->CanCreateConnection(Smith->Pins[2], Miner->Pins[0]).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("拒绝重复双向增益"), Schema->CanCreateConnection(Warrior->Pins[2], Smith->Pins[1]).Response, CONNECT_RESPONSE_DISALLOW);
	TestEqual(TEXT("已存在关系不能重复连接"), Schema->CanCreateConnection(Miner->Pins[2], Smith->Pins[0]).Response, CONNECT_RESPONSE_DISALLOW);
	FLxProfessionTreePlaceAction DuplicateAction; DuplicateAction.Definition = Warrior->Definition;
	DuplicateAction.PerformAction(Graph, nullptr, FVector2f(100, 100));
	TestEqual(TEXT("同一个职业不重复放置"), Graph->Nodes.Num(), 6);
	ULxProfessionTreeInfluence* Mutual = Asset->GetIncomingInfluences(Mage->Definition)[0];
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("职业测试修改倍率")));
		Mutual->Modify(); Mutual->Multiplier = 0.f;
	}
	TestTrue(TEXT("零倍率是有效的完全抑制"), Asset->ValidateConfiguration().IsEmpty());
	GEditor->UndoTransaction(); TestEqual(TEXT("撤销恢复抑制倍率"), Mutual->Multiplier, 0.01f);
	GEditor->RedoTransaction(); TestEqual(TEXT("重做恢复零倍率"), Mutual->Multiplier, 0.f);
	GEditor->UndoTransaction();
	Graph->SynchronizeAsset();
	TestTrue(TEXT("同步图不替换原关系参数对象"), Asset->Influences.Contains(Mutual));
	TestEqual(TEXT("同步后倍率不被重置"), Mutual->Multiplier, 0.01f);
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("职业测试断开关系")));
		Schema->BreakSinglePinLink(Mage->Pins[2], Warrior->Pins[1]);
	}
	TestEqual(TEXT("断线立即移除静态影响"), Asset->Influences.Num(), 3);
	GEditor->UndoTransaction();
	TestEqual(TEXT("撤销断线恢复静态关系"), Asset->Influences.Num(), 4);
	TestTrue(TEXT("撤销恢复原来连线"), Mage->Pins[2]->LinkedTo.Contains(Warrior->Pins[1]));
	TestEqual(TEXT("撤销断线保留抑制参数"), Mutual->Multiplier, 0.01f);

	ULxProfessionTreeAsset* Copy = DuplicateObject<ULxProfessionTreeAsset>(Asset, GetTransientPackage());
	TestNotEqual(TEXT("复制资产不共享职业配置"), Copy->Professions[0].Get(), Asset->Professions[0].Get());
	TestTrue(TEXT("复制关系引用复制后的定义"), Copy->Professions.Contains(Copy->Influences[0]->Source));
	CastChecked<ULxProfessionTreeEdGraph>(Copy->EditorGraph)->SynchronizeAsset();
	TestEqual(TEXT("复制后连线保留"), Copy->Influences.Num(), 4);
	Copy->Professions[0]->FirstLevelExperience = 999;
	Copy->Professions[0]->PostEditChange();
	TestEqual(TEXT("副本公式编辑不修改原资产"), Asset->Professions[0]->GetRequiredExperienceForNextLevel(1), 100.f);
	Copy->ExperienceGrowthCoefficient = 0.75f;
	Copy->PostEditChange();
	TestEqual(TEXT("副本职业使用所属资产的增长系数"), Copy->Professions[1]->CalculateLevelExperience(3), 400.f);
	TestEqual(TEXT("副本增长系数不影响原资产"), Asset->Professions[1]->CalculateLevelExperience(3), 200.f);

	const FString TestDirectory = FPaths::ProjectSavedDir() / TEXT("Tests");
	IFileManager::Get().MakeDirectory(*TestDirectory, true);
	UPackage* Package = CreatePackage(TEXT("/Temp/职业关系保存测试"));
	ULxProfessionTreeAsset* Saved = DuplicateObject<ULxProfessionTreeAsset>(Asset, Package, TEXT("职业关系保存测试"));
	Saved->SetFlags(RF_Public | RF_Standalone);
	Saved->Professions[0]->FirstLevelExperience = 120.f;
	Saved->ExperienceGrowthCoefficient = 0.5f;
	Saved->PostEditChange();
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = TestDirectory / TEXT("职业关系保存测试.uasset");
	TestTrue(TEXT("关系资产保存成功"), UPackage::SavePackage(Package, Saved, *Filename, SaveArgs));
	UPackage* LoadedPackage = LoadPackage(CreatePackage(TEXT("/Temp/职业关系重载测试")), *Filename, LOAD_None);
	auto* Loaded = LoadedPackage ? FindObject<ULxProfessionTreeAsset>(LoadedPackage, TEXT("职业关系保存测试")) : nullptr;
	if (TestNotNull(TEXT("关系资产从磁盘重新载入"), Loaded))
	{
		TestTrue(TEXT("重载数据有效"), Loaded->ValidateConfiguration().IsEmpty());
		TestEqual(TEXT("重载后关系数量"), Loaded->Influences.Num(), 4);
		TestEqual(TEXT("重载后职业等级数量"), Loaded->Professions[0]->Levels.Num(), 5);
		TestEqual(TEXT("重载后保留公式起点"), Loaded->Professions[0]->FirstLevelExperience, 120.f);
		TestEqual(TEXT("重载后保留体系增长系数"), Loaded->ExperienceGrowthCoefficient, 0.5f);
		TestEqual(TEXT("重载后其他职业共用体系增长系数"), Loaded->Professions[1]->Levels[2].RequiredExperience, 300.f);
		TestEqual(TEXT("重载后按公式计算第三级经验"), Loaded->Professions[0]->GetRequiredExperienceForNextLevel(3), 360.f);
		TestTrue(TEXT("重载后图节点指向同资产职业"), CastChecked<ULxProfessionTreeEdGraphNode>(Loaded->EditorGraph->Nodes[0])->Definition == Loaded->Professions[0]);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CreateProfessionTreeExample")))
	{
		const FString Folder = FPaths::ProjectContentDir() / TEXT("职业");
		const FString ExampleFile = Folder / TEXT("职业关系示例.uasset");
		if (!IFileManager::Get().FileExists(*ExampleFile))
		{
			IFileManager::Get().MakeDirectory(*Folder, true);
			UPackage* ExamplePackage = CreatePackage(TEXT("/Game/职业/职业关系示例"));
			auto* Example = DuplicateObject<ULxProfessionTreeAsset>(Asset, ExamplePackage, TEXT("职业关系示例"));
			Example->SetFlags(RF_Public | RF_Standalone);
			TestTrue(TEXT("生成可打开的中文职业关系示例"), UPackage::SavePackage(ExamplePackage, Example, *ExampleFile, SaveArgs));
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("SmokeProfessionTreeEditor")) && FApp::CanEverRender())
	{
		auto Editor = MakeShared<FLxProfessionTreeAssetEditor>();
		// 预览测试使用临时样本，用户移动或编辑示例资产不会影响测试，也不会被测试修改。
		auto* PreviewAsset = Asset;
		Editor->Init(EToolkitMode::Standalone, nullptr, PreviewAsset);
		if (TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(Editor->GetToolkitHost()->GetParentWidget()))
			Window->Resize(FVector2f(1920, 1080));
		if (PreviewAsset->Professions.IsValidIndex(4)) Editor->SelectProfession(PreviewAsset->Professions[4]);
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Editor, TestDirectory]()
		{
			TArray<FColor> Pixels; FIntVector Size;
			if (FSlateApplication::Get().TakeScreenshot(Editor->GetToolkitHost()->GetParentWidget(), Pixels, Size))
			{
				TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
				FFileHelper::SaveArrayToFile(Png, *(TestDirectory / TEXT("职业关系编辑器.png")));
			}
			Editor->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed);
		}, 3.0f));
	}
	return true;
}

/** 防止刷新回调循环销毁节点控件，导致鼠标选择和拖动失效。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfessionTreeInteractionTest, "LxARPG.ProfessionTree.InteractionStability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实编辑器和Slate节点验证选择、连续移动、属性刷新和撤销后的稳定性。 */
bool FLxProfessionTreeInteractionTest::RunTest(const FString& Parameters)
{
	auto* Asset = MakeExample();
	auto* Graph = CastChecked<ULxProfessionTreeEdGraph>(Asset->EditorGraph);
	auto Editor = MakeShared<FLxProfessionTreeAssetEditor>();
	Editor->Init(EToolkitMode::Standalone, nullptr, Asset);
	SGraphPanel* Panel = Editor->GraphEditor->GetGraphPanel();
	if (!TestNotNull(TEXT("真实关系画布已创建"), Panel))
	{ Editor->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed); return false; }
	Panel->Update();
	Editor->SelectProfession(Asset->Professions[2]);
	TestEqual(TEXT("矿工面板只显示收到的增益，不显示对铁匠的前置"), Editor->InfluenceBox->GetChildren()->Num(), 1);
	Editor->SelectProfession(Asset->Professions[4]);
	TestEqual(TEXT("铁匠面板包含前置和相互影响的入向增益"), Editor->InfluenceBox->GetChildren()->Num(), 2);
	ULxProfessionTreeInfluence* SharedBenefit = nullptr;
	for (ULxProfessionTreeInfluence* Relation : Asset->Influences)
		if (Relation->Source == Asset->Professions[4]) SharedBenefit = Relation;
	if (TestNotNull(TEXT("铁匠相互影响关系存在"), SharedBenefit))
	{
		SharedBenefit->bMutual = false; SharedBenefit->PostEditChange(); Editor->Tick(1.f / 60.f);
		TestEqual(TEXT("关闭相互影响后输出关系从来源面板消失"), Editor->InfluenceBox->GetChildren()->Num(), 1);
		SharedBenefit->bMutual = true; SharedBenefit->PostEditChange(); Editor->Tick(1.f / 60.f);
		TestEqual(TEXT("重新启用后恢复入向关系"), Editor->InfluenceBox->GetChildren()->Num(), 2);
	}
	int32 Notifications = 0;
	const FDelegateHandle Handle = Graph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateLambda(
		[&Notifications](const FEdGraphEditAction&) { ++Notifications; }));
	for (int32 Frame = 0; Frame < 8; ++Frame) Editor->Tick(1.f / 60.f);
	TestEqual(TEXT("空闲刷新不得重新广播画布变化"), Notifications, 0);
	// Update本身会创建节点控件，只在测试准备阶段补齐一次，不能在移动循环里人为重建。
	Panel->Update();

	auto* Node = CastChecked<ULxProfessionTreeEdGraphNode>(Graph->Nodes[4]);
	TSharedPtr<SGraphNode> Widget = Panel->GetNodeWidgetFromGuid(Node->NodeGuid);
	if (TestTrue(TEXT("节点可视控件存在"), Widget.IsValid()))
	{
		Editor->GraphEditor->ClearSelectionSet();
		Editor->GraphEditor->SetNodeSelection(Node, true);
		TestTrue(TEXT("画布选择联动职业细节"), Editor->SelectedProfession.Get() == Node->Definition);
		TestTrue(TEXT("跨分类选择联动列表"), Editor->ProfessionList->GetSelectedItems().Contains(Node->Definition.Get()));
		const FVector2f Start(Node->NodePosX, Node->NodePosY);
		{
			const FScopedTransaction Transaction(FText::FromString(TEXT("职业节点连续拖动测试")));
			for (int32 Step = 1; Step <= 5; ++Step)
			{
				SNodePanel::SNode::FNodeSet MovedNodes;
				Widget->MoveTo(Start + FVector2f(32.f * Step, 16.f * Step), MovedNodes);
				Editor->Tick(1.f / 60.f);
				TestTrue(TEXT("连续移动期间保留原节点控件"), Panel->GetNodeWidgetFromGuid(Node->NodeGuid) == Widget);
				TestTrue(TEXT("连续移动期间保留选择"), Editor->GraphEditor->GetSelectedNodes().Contains(Node));
			}
		}
		TestEqual(TEXT("连续移动保存横坐标"), Node->NodePosX, FMath::RoundToInt(Start.X + 160.f));
		TestEqual(TEXT("连续移动保存纵坐标"), Node->NodePosY, FMath::RoundToInt(Start.Y + 80.f));
		TestEqual(TEXT("移动不改变职业关系数量"), Asset->Influences.Num(), 4);
		GEditor->UndoTransaction();
		TestEqual(TEXT("撤销移动恢复横坐标"), Node->NodePosX, FMath::RoundToInt(Start.X));
		Editor->Tick(1.f / 60.f); Panel->Update();
	}

	// 属性确实需要更新标题，但一次属性变更不能触发无穷画布通知。
	Notifications = 0;
	Node->Definition->DisplayName = FText::FromString(TEXT("铁匠改名测试"));
	Node->Definition->PostEditChange();
	for (int32 Frame = 0; Frame < 8; ++Frame) Editor->Tick(1.f / 60.f);
	TestEqual(TEXT("职业属性变更只刷新画布一次"), Notifications, 1);
	TestTrue(TEXT("重命名后节点显示新名称"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(TEXT("铁匠改名测试")));
	TestFalse(TEXT("属性刷新最终回到空闲"), Editor->bRefreshPending);
	Graph->RemoveOnGraphChangedHandler(Handle);
	Editor->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed);
	return true;
}

/** 覆盖经验起点、整体缩放、增长曲率、满级边界及旧等级结构兼容。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfessionExperienceFormulaTest, "LxARPG.ProfessionTree.ExperienceFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 用明确数值及不变量验证二次经验公式，而非依赖手工经验配置。 */
bool FLxProfessionExperienceFormulaTest::RunTest(const FString& Parameters)
{
	auto* Asset = MakeExample();
	ULxProfessionTreeDefinition* Definition = Asset->Professions[0];
	const float Expected[] = { 100.f, 125.f, 200.f, 325.f, 500.f };
	for (int32 Level = 1; Level <= 5; ++Level)
		TestEqual(TEXT("默认二次经验曲线符合示例数值"), Definition->CalculateLevelExperience(Level), Expected[Level - 1]);
	TestEqual(TEXT("未学习等级没有升级需求"), Definition->GetRequiredExperienceForNextLevel(0), 0.f);
	TestEqual(TEXT("负等级没有经验需求"), Definition->CalculateLevelExperience(-1), 0.f);
	TestEqual(TEXT("满级不再升级"), Definition->GetRequiredExperienceForNextLevel(5), 0.f);
	TestEqual(TEXT("越界等级不再升级"), Definition->GetRequiredExperienceForNextLevel(6), 0.f);
	Definition->FirstLevelExperience = 200.f;
	for (int32 Level = 1; Level <= 5; ++Level)
		TestEqual(TEXT("y翻倍使整条曲线按比例翻倍"), Definition->CalculateLevelExperience(Level), Expected[Level - 1] * 2.f);
	Asset->ExperienceGrowthCoefficient = 0.5f;
	TestEqual(TEXT("改变z不改变第一级经验"), Definition->CalculateLevelExperience(1), 200.f);
	TestEqual(TEXT("增大z加快后期增长"), Definition->CalculateLevelExperience(5), 1800.f);
	for (int32 Level = 2; Level < 5; ++Level)
		TestTrue(TEXT("相邻等级经验差逐渐增大"), Definition->CalculateLevelExperience(Level + 1) - Definition->CalculateLevelExperience(Level)
			> Definition->CalculateLevelExperience(Level) - Definition->CalculateLevelExperience(Level - 1));
	Asset->ExperienceGrowthCoefficient = 0.f;
	TestEqual(TEXT("z为0时保持固定经验"), Definition->CalculateLevelExperience(100), 200.f);
	Asset->ExperienceGrowthCoefficient = -1.f;
	TestFalse(TEXT("负增长系数不能通过配置检查"), Asset->ValidateConfiguration().IsEmpty());
	Asset->ExperienceGrowthCoefficient = 0.25f;
	Definition->FirstLevelExperience = 0.f;
	TestFalse(TEXT("零起始经验不能通过配置检查"), Asset->ValidateConfiguration().IsEmpty());
	Definition->FirstLevelExperience = MAX_flt;
	Asset->ExperienceGrowthCoefficient = MAX_flt;
	TestTrue(TEXT("极端参数保持有限经验值"), FMath::IsFinite(Definition->CalculateLevelExperience(MAX_int32)));
	Definition->FirstLevelExperience = 100.f; Asset->ExperienceGrowthCoefficient = 0.25f;
	const FGameplayTag OriginalEntry = Definition->Levels[1].LevelEntryQuotes[0].EntryID;
	Definition->Levels[1].RequiredExperience = 9999.f;
	FLxProfessionLevelEffectConfig Config;
	TestTrue(TEXT("可查询已有等级配置"), Definition->GetLevelEffectConfig(2, Config));
	TestEqual(TEXT("等级查询忽略旧手填经验"), Config.RequiredExperience, 125.f);
	Definition->PostEditChange();
	TestEqual(TEXT("编辑后自动更新经验缓存"), Definition->Levels[1].RequiredExperience, 125.f);
	TestEqual(TEXT("计算经验不改变原来的词条"), Definition->Levels[1].LevelEntryQuotes[0].EntryID, OriginalEntry);
	TestEqual(TEXT("原满级缓存为0"), Definition->Levels.Last().RequiredExperience, 0.f);
	Definition->Levels.AddDefaulted(); Definition->PostEditChange();
	TestEqual(TEXT("增加等级后原满级自动获得升级需求"), Definition->Levels[4].RequiredExperience, 500.f);
	TestEqual(TEXT("新满级缓存为0"), Definition->Levels.Last().RequiredExperience, 0.f);
	TestFalse(TEXT("越界等级不能取得加成配置"), Definition->GetLevelEffectConfig(7, Config));
	// 同一体系共用增长系数，每个职业保留独立起点；撤销和重做必须同步刷新全部缓存。
	Asset->Professions[1]->FirstLevelExperience = 200.f;
	Asset->PostEditChange();
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("测试体系经验增长系数")));
		Asset->Modify();
		Asset->ExperienceGrowthCoefficient = 0.5f;
		Asset->PostEditChange();
	}
	for (const ULxProfessionTreeDefinition* Profession : Asset->Professions)
		TestEqual(TEXT("体系修改同步全部职业缓存"), Profession->Levels[2].RequiredExperience, Profession->FirstLevelExperience * 3.f);
	TestEqual(TEXT("体系修改保留不同职业的独立起点"), Asset->Professions[1]->GetRequiredExperienceForNextLevel(1), 200.f);
	GEditor->UndoTransaction();
	TestEqual(TEXT("撤销恢复体系系数"), Asset->ExperienceGrowthCoefficient, 0.25f);
	for (const ULxProfessionTreeDefinition* Profession : Asset->Professions)
		TestEqual(TEXT("撤销同步全部职业缓存"), Profession->Levels[2].RequiredExperience, Profession->FirstLevelExperience * 2.f);
	GEditor->RedoTransaction();
	for (const ULxProfessionTreeDefinition* Profession : Asset->Professions)
		TestEqual(TEXT("重做同步全部职业缓存"), Profession->Levels[2].RequiredExperience, Profession->FirstLevelExperience * 3.f);
	return true;
}
/** 验证入向关系过滤以及前置等级上下界和来源等级变化。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfessionIncomingInfluenceTest, "LxARPG.ProfessionTree.IncomingInfluences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 单向关系仅作用于接收方，前置上限始终以来源职业为准。 */
bool FLxProfessionIncomingInfluenceTest::RunTest(const FString& Parameters)
{
	auto* Asset = MakeExample();
	auto* Miner = Asset->Professions[2].Get();
	auto* Smith = Asset->Professions[4].Get();
	TestEqual(TEXT("纯输出职业没有入向影响"), Asset->GetIncomingInfluences(Asset->Professions[3]).Num(), 0);
	TestEqual(TEXT("输出前置不作为来源职业自身的影响"), Asset->GetIncomingInfluences(Miner).Num(), 1);
	ULxProfessionTreeInfluence* Prerequisite = nullptr;
	for (ULxProfessionTreeInfluence* Relation : Asset->GetIncomingInfluences(Smith))
		if (Relation->Kind == ELxProfessionTreeRelation::Prerequisite) Prerequisite = Relation;
	if (!TestNotNull(TEXT("前置关系存在"), Prerequisite)) return false;
	Smith->Levels.SetNum(10);
	TestEqual(TEXT("上限使用来源等级而非目标等级"), Prerequisite->GetMaxRequiredLevel(), 5);
	Prerequisite->SetRequiredLevel(-10);
	TestEqual(TEXT("前置等级最低为1"), Prerequisite->RequiredLevel, 1);
	Prerequisite->SetRequiredLevel(100);
	TestEqual(TEXT("前置等级不能超过来源最高等级"), Prerequisite->RequiredLevel, 5);
	Prerequisite->SetRequiredLevel(3);
	TestEqual(TEXT("合法前置要求保持原值"), Prerequisite->RequiredLevel, 3);
	Prerequisite->RequiredLevel = 99; Prerequisite->PostEditChange();
	TestEqual(TEXT("属性编辑同样约束上限"), Prerequisite->RequiredLevel, 5);
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("测试前置来源缩减等级")));
		Miner->Modify(); Miner->Levels.SetNum(2); Miner->PostEditChange();
	}
	TestEqual(TEXT("来源缩减等级同步收紧要求"), Prerequisite->RequiredLevel, 2);
	GEditor->UndoTransaction();
	TestEqual(TEXT("撤销恢复来源等级"), Miner->Levels.Num(), 5);
	TestEqual(TEXT("撤销恢复前置要求"), Prerequisite->RequiredLevel, 5);
	Miner->Levels.Reset(); Miner->PostEditChange();
	TestEqual(TEXT("空来源没有可配置上限"), Prerequisite->GetMaxRequiredLevel(), 0);
	TestFalse(TEXT("空来源等级配置仍报告错误"), Asset->ValidateConfiguration().IsEmpty());
	return true;
}
#endif
