#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Toolkits/IToolkitHost.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "EdGraph/EdGraphPin.h"
#include "LxInteractionTreeAssetFactory.h"
#include "LxInteractionTreeAssetEditor.h"
#include "LxInteractionTreeEdGraph.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "Editor.h"

/** 覆盖创建规则、图结构、运行时隔离、资产复制和编辑器打开。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionTreeAssetTest, "LxARPG.InteractionTree.AssetAndEditor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过实际图操作和组件实例验证从编辑到运行时的完整路径。 */
bool FLxInteractionTreeAssetTest::RunTest(const FString& Parameters)
{
	ULxInteractionTreeAssetFactory* Factory = NewObject<ULxInteractionTreeAssetFactory>();
	ULxInteractionTreeAsset* Asset = CastChecked<ULxInteractionTreeAsset>(Factory->FactoryCreateNew(
		ULxInteractionTreeAsset::StaticClass(), GetTransientPackage(), NAME_None, RF_Transactional, nullptr, GWarn));
	ULxInteractionTreeEdGraph* Graph = CastChecked<ULxInteractionTreeEdGraph>(Asset->EditorGraph);
	Graph->EnsureStartNode();
	TestEqual(TEXT("重复初始化仍只有一个开始节点"), Graph->Nodes.Num(), 1);
	ULxInteractionTreeEdGraphNode* Start = CastChecked<ULxInteractionTreeEdGraphNode>(Graph->Nodes[0]);
	TestFalse(TEXT("开始节点不能删除"), Start->CanUserDeleteNode());
	TestEqual(TEXT("开始节点只有子项引脚"), Start->Pins.Num(), 1);
	TestFalse(TEXT("未启用仓库不能放置"), Asset->CanAddNode(ELxInteractionActionType::Warehouse));

	/** 通过实际右键菜单操作创建节点并自动连接。 */
	auto Add = [Graph](ELxInteractionActionType Type, UEdGraphPin* From, float X, float Y)
	{
		FLxInteractionTreeNewNodeAction Action;
		Action.Type = Type;
		return Cast<ULxInteractionTreeEdGraphNode>(Action.PerformAction(Graph, From, FVector2f(X, Y), false));
	};
	ULxInteractionTreeEdGraphNode* Entrance = Add(ELxInteractionActionType::Entrance, Start->Pins[0], 300, 0);
	if (!TestNotNull(TEXT("开始后创建入口"), Entrance)) return false;
	Entrance->Data->PromptText = FText::FromString(TEXT("与村民交谈"));
	Entrance->Data->NpcDialogueText = FText::FromString(TEXT("有什么可以帮你？"));
	ULxInteractionTreeEdGraphNode* Dialogue = Add(ELxInteractionActionType::Dialogue, Entrance->Pins[1], 650, -180);
	if (!TestNotNull(TEXT("创建对话"), Dialogue)) return false;
	TestFalse(TEXT("新建普通对话默认不关闭交互"), Dialogue->Data->bCloseInteractionDialogue);
	Dialogue->Data->PromptText = FText::FromString(TEXT("打听附近的消息"));
	Dialogue->Data->NpcDialogueText = FText::FromString(TEXT("村外最近不太平，出发前记得整理装备。"));
	ULxInteractionTreeEdGraphNode* Exit = Add(ELxInteractionActionType::InteractionExit, Entrance->Pins[1], 650, 220);
	if (!TestNotNull(TEXT("创建退出"), Exit)) return false;
	Exit->Data->PromptText = FText::FromString(TEXT("下次再聊"));
	Exit->Data->OptionOrder = 100;
	TestEqual(TEXT("退出只有父项引脚"), Exit->Pins.Num(), 1);
	Asset->Features.bEnableWarehouse = true;
	ULxInteractionTreeEdGraphNode* Warehouse = Add(ELxInteractionActionType::Warehouse, Entrance->Pins[1], 650, 20);
	if (!TestNotNull(TEXT("启用后允许放置仓库"), Warehouse)) return false;
	TestTrue(TEXT("新建功能节点默认关闭对话窗口"), Warehouse->Data->bCloseInteractionDialogue);
	Warehouse->Data->PromptText = FText::FromString(TEXT("打开我的仓库"));
	TestFalse(TEXT("同类功能默认只能放一个"), Asset->CanAddNode(ELxInteractionActionType::Warehouse));
	TestNull(TEXT("直接执行菜单操作也不能绕过数量限制"), Add(ELxInteractionActionType::Warehouse, nullptr, 950, 0));
	TestTrue(TEXT("节点标题直接包含提示文本"), Warehouse->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(TEXT("打开我的仓库")));
	TestTrue(TEXT("节点标题包含中文类型"), Warehouse->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(TEXT("仓库")));

	const UEdGraphSchema* Schema = Graph->GetSchema();
	TestEqual(TEXT("拒绝第二个父项"), Schema->CanCreateConnection(Start->Pins[0], Warehouse->Pins[0]).Response, CONNECT_RESPONSE_DISALLOW);
	Schema->BreakSinglePinLink(Entrance->Pins[0], Start->Pins[0]);
	TestEqual(TEXT("入口不能成为普通节点的子项"), Schema->CanCreateConnection(Dialogue->Pins[1], Entrance->Pins[0]).Response, CONNECT_RESPONSE_DISALLOW);
	Schema->TryCreateConnection(Start->Pins[0], Entrance->Pins[0]);
	// 对话子节点形成回路时，父项引脚已断开，确保确实命中循环检查。
	ULxInteractionTreeEdGraphNode* Nested = Add(ELxInteractionActionType::Dialogue, Dialogue->Pins[1], 1000, -180);
	Schema->BreakSinglePinLink(Entrance->Pins[1], Dialogue->Pins[0]);
	TestEqual(TEXT("拒绝循环"), Schema->CanCreateConnection(Nested->Pins[1], Dialogue->Pins[0]).Response, CONNECT_RESPONSE_DISALLOW);
	Schema->TryCreateConnection(Entrance->Pins[1], Dialogue->Pins[0]);
	Graph->SynchronizeAsset();
	FText Error;
	TestTrue(TEXT("正确连线通过运行时校验"), Asset->ValidateTree(Error));
	Asset->Features.bEnableWarehouse = false;
	TestFalse(TEXT("已有功能被关闭时资产校验提示错误"), Asset->ValidateTree(Error));
	Asset->Features.bEnableWarehouse = true;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	AActor* NpcA = World->SpawnActor<AActor>();
	AActor* NpcB = World->SpawnActor<AActor>();
	ULxInteractableComponent* ComponentA = NewObject<ULxInteractableComponent>(NpcA);
	ULxInteractableComponent* ComponentB = NewObject<ULxInteractableComponent>(NpcB);
	ComponentA->FeatureConfig.bEnableWarehouse = true;
	ComponentA->FeatureConfig.WarehouseConfig.SlotCount = 12;
	ComponentB->FeatureConfig.bEnableWarehouse = true;
	ComponentB->FeatureConfig.WarehouseConfig.SlotCount = 24;
	ComponentA->RegisterComponentWithWorld(World);
	ComponentB->RegisterComponentWithWorld(World);
	if (!TestTrue(TEXT("首个NPC加载交互树资产"), ComponentA->SetInteractionTreeAsset(Asset))
		|| !TestTrue(TEXT("另一个NPC加载相同资产"), ComponentB->SetInteractionTreeAsset(Asset))) return false;
	ComponentA->BaseComponentInitialize();
	ComponentB->BaseComponentInitialize();
	TestEqual(TEXT("组件仅指定资产即可加载一个入口"), ComponentA->GetRootInteractionNodes().Num(), 1);
	TestEqual(TEXT("每个NPC均创建仓库功能"), ComponentA->GetInteractionFeatures().Num(), 1);
	for (ULxInteractionNode* Child : ComponentA->GetRootInteractionNodes()[0]->GetChildNodes())
	{
		if (Child->GetInteractionActionType() == ELxInteractionActionType::Warehouse)
		{
			TestTrue(TEXT("静态关闭配置传递到运行时功能节点"), Child->ShouldCloseInteractionDialogue());
		}
		if (Child->GetInteractionActionType() == ELxInteractionActionType::InteractionExit)
		{
			TestTrue(TEXT("退出节点始终关闭交互"), Child->ShouldCloseInteractionDialogue());
		}
	}
	if (ComponentA->GetRootInteractionNodes().IsEmpty() || ComponentA->GetInteractionFeatures().IsEmpty()) return false;
	TestNotEqual(TEXT("NPC之间不共享运行时节点"), ComponentA->GetRootInteractionNodes()[0], ComponentB->GetRootInteractionNodes()[0]);
	TestNotEqual(TEXT("NPC之间不共享功能实例"), ComponentA->GetInteractionFeatures()[0], ComponentB->GetInteractionFeatures()[0]);
	ULxWarehouseInteractionComponent* WarehouseFeature = Cast<ULxWarehouseInteractionComponent>(ComponentA->GetInteractionFeatures()[0]);
	if (!TestNotNull(TEXT("功能类型正确"), WarehouseFeature)) return false;
	TArray<ULxItemSlotData*> Slots;
	WarehouseFeature->GetWarehouseItemSlotList(Slots);
	TestEqual(TEXT("组件配置被应用到独立仓库"), Slots.Num(), 12);
	const ULxInteractionActionComponentBase* OriginalFeature = ComponentA->GetInteractionFeatures()[0];
	ComponentA->InitializeInteractionFeatures();
	TestTrue(TEXT("重复初始化不重置功能状态"), OriginalFeature == ComponentA->GetInteractionFeatures()[0]);
	TestTrue(TEXT("重复指定同一资产成功"), ComponentA->SetInteractionTreeAsset(Asset));
	TestTrue(TEXT("重复指定同一资产不重建功能"), OriginalFeature == ComponentA->GetInteractionFeatures()[0]);
	TestEqual(TEXT("运行时按选项顺序排列"), ComponentA->GetRootInteractionNodes()[0]->GetChildNodes().Last()->GetPromptText().ToString(), FString(TEXT("下次再聊")));
	World->SetBegunPlay(true);
	NpcA->DispatchBeginPlay();
	USphereComponent* Range = NpcA->FindComponentByClass<USphereComponent>();
	TestNotNull(TEXT("仅配置资产自动创建交互范围"), Range);
	if (Range) TestEqual(TEXT("范围半径采用组件配置"), Range->GetUnscaledSphereRadius(), ComponentA->AssetInteractionRangeRadius);
	TestTrue(TEXT("开始运行不重新创建已初始化功能"), OriginalFeature == ComponentA->GetInteractionFeatures()[0]);
	ComponentA->SetInteractionRangeColliders({});
	TestTrue(TEXT("清空范围绑定后重新加载资产"), ComponentA->LoadInteractionTreeAsset());
	TestTrue(TEXT("重载复用原自动范围球体"), NpcA->FindComponentByClass<USphereComponent>() == Range);
	TArray<USphereComponent*> RangeComponents;
	NpcA->GetComponents(RangeComponents);
	TestEqual(TEXT("重载不会遗留重复范围球体"), RangeComponents.Num(), 1);
	ComponentA->SetInteractionTreeAsset(nullptr);
	TestTrue(TEXT("卸载后删除运行时根节点"), ComponentA->GetRootInteractionNodes().IsEmpty());
	TestNull(TEXT("卸载后销毁自动交互范围"), NpcA->FindComponentByClass<USphereComponent>());
	TestEqual(TEXT("卸载当前NPC不影响另一个NPC的仓库"), ComponentB->GetInteractionFeatures().Num(), 1);
	TestTrue(TEXT("运行中重新指定资产成功"), ComponentA->SetInteractionTreeAsset(Asset));
	TestNotNull(TEXT("运行中重新指定资产自动恢复交互范围"), NpcA->FindComponentByClass<USphereComponent>());

	// 复制整个资产时，图节点必须引用复制后的配置对象。
	ULxInteractionTreeAsset* Copy = DuplicateObject<ULxInteractionTreeAsset>(Asset, GetTransientPackage());
	TestNotEqual(TEXT("复制资产拥有独立静态配置"), Copy->Nodes[0].Get(), Asset->Nodes[0].Get());
	CastChecked<ULxInteractionTreeEdGraph>(Copy->EditorGraph)->SynchronizeAsset();
	TestTrue(TEXT("复制后图连接保持有效"), Copy->ValidateTree(Error));
	TestEqual(TEXT("复制后仍保留全部节点"), Copy->Nodes.Num(), Asset->Nodes.Num());

	// 以独立包保存并重新加载，确认运行时数据不依赖打开编辑器。
	const FString PackageName = TEXT("/Temp/交互树保存测试");
	UPackage* Package = CreatePackage(*PackageName);
	ULxInteractionTreeAsset* SavedAsset = DuplicateObject<ULxInteractionTreeAsset>(Asset, Package, TEXT("交互树保存测试"));
	SavedAsset->SetFlags(RF_Public | RF_Standalone);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const FString File = FPaths::ProjectSavedDir() / TEXT("Tests/交互树保存测试.uasset");
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Tests")), true);
	TestTrue(TEXT("资产能保存到磁盘"), UPackage::SavePackage(Package, SavedAsset, *File, SaveArgs));
	// 直接文件路径加载到独立外层包。
	UPackage* LoadOuter = CreatePackage(TEXT("/Temp/交互树磁盘副本"));
	UPackage* Loaded = LoadPackage(LoadOuter, *File, LOAD_None);
	ULxInteractionTreeAsset* LoadedAsset = Loaded ? FindObject<ULxInteractionTreeAsset>(Loaded, TEXT("交互树保存测试")) : nullptr;
	TestNotNull(TEXT("从磁盘重新读取资产"), LoadedAsset);
	if (LoadedAsset) TestTrue(TEXT("重读后连线及功能配置有效"), LoadedAsset->ValidateTree(Error));

	if (FParse::Param(FCommandLine::Get(), TEXT("CreateInteractionTreeExample")))
	{
		const FString ExampleFile = FPaths::ProjectContentDir() / TEXT("交互/交互示例.uasset");
		if (!IFileManager::Get().FileExists(*ExampleFile))
		{
			UPackage* ExamplePackage = CreatePackage(TEXT("/Game/交互/交互示例"));
			ULxInteractionTreeAsset* Example = DuplicateObject<ULxInteractionTreeAsset>(Asset, ExamplePackage, TEXT("交互示例"));
			Example->SetFlags(RF_Public | RF_Standalone);
			IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("交互")), true);
			TestTrue(TEXT("创建可直接打开的中文交互示例资产"), UPackage::SavePackage(ExamplePackage, Example, *ExampleFile, SaveArgs));
		}
	}
	// 可选无交互打开测试，验证细节委托与Slate布局构建。
	if (FParse::Param(FCommandLine::Get(), TEXT("SmokeInteractionTreeEditor")) && FApp::CanEverRender())
	{
		TSharedRef<FLxInteractionTreeAssetEditor> Editor = MakeShared<FLxInteractionTreeAssetEditor>();
		Editor->Init(EToolkitMode::Standalone, nullptr, Asset);
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Editor]()
		{
		TArray<FColor> Pixels;
		FIntVector Size;
		if (FSlateApplication::Get().TakeScreenshot(Editor->GetToolkitHost()->GetParentWidget(), Pixels, Size))
		{
			TArray<uint8> Png;
			FImageUtils::CompressImageArray(Size.X, Size.Y, Pixels, Png);
			FFileHelper::SaveArrayToFile(Png, *(FPaths::ProjectSavedDir() / TEXT("Tests/交互树编辑器.png")));
		}
		Editor->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed);
		}, 1.0f));
	}
	return true;
}

#endif
