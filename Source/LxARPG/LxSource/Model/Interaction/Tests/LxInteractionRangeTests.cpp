#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionUIManager.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"
#include "UObject/UnrealType.h"

/** 验证树节点和NPC开关共同决定真实玩家选项及执行资格。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionComponentFeatureGateTest,
	"LxARPG.Interaction.ComponentFeatureGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 用实际玩家模块和对话控件覆盖根功能、子功能及缺少节点的组合。 */
bool FLxInteractionComponentFeatureGateTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建功能开关测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	AActor* Npc = World->SpawnActor<AActor>();
	ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建NPC"), Npc) || !TestNotNull(TEXT("创建玩家"), Player)) return false;
	ULxPlayerInteractionModule* Receiver = Player->GetPlayerInteractionComponent();
	Receiver->InitializeModule(Player->GetPlayerControlComponent());
	ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Npc);
	Provider->bCreateAssetInteractionRange = false;
	Provider->RegisterComponentWithWorld(World);
	ULxDialogueInteractionWidget* Widget = NewObject<ULxDialogueInteractionWidget>();
	Widget->SetPlayerInteractionComponent(Receiver);
	ON_SCOPE_EXIT { Widget->SetPlayerInteractionComponent(nullptr); };
	ULxInteractionEntranceWidget* EntranceWidget = NewObject<ULxInteractionEntranceWidget>();
	EntranceWidget->SetPlayerInteractionComponent(Receiver);
	ON_SCOPE_EXIT { EntranceWidget->SetPlayerInteractionComponent(nullptr); };
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	Asset->Features.bEnableWarehouse = true;
	ULxInteractionTreeNodeData* Root = NewObject<ULxInteractionTreeNodeData>(Asset);
	Root->NodeId = FGuid::NewGuid();
	Root->Type = ELxInteractionActionType::Entrance;
	Root->PromptText = FText::FromString(TEXT("交谈"));
	ULxInteractionTreeNodeData* Warehouse = NewObject<ULxInteractionTreeNodeData>(Asset);
	Warehouse->NodeId = FGuid::NewGuid();
	Warehouse->Type = ELxInteractionActionType::Warehouse;
	Warehouse->PromptText = FText::FromString(TEXT("仓库"));
	Root->Children.Add(Warehouse->NodeId);
	Asset->Nodes = {Root, Warehouse};
	Asset->Roots = {Root->NodeId};
	if (!TestTrue(TEXT("组件默认关闭仍允许加载有效树"), Provider->SetInteractionTreeAsset(Asset))) return false;
	Provider->HandleInteractionRangeBeginOverlap(Player);
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("树有节点但组件关闭时不显示子选项"), Widget->GetDialogueOptionCount(), 0);
	ULxInteractionNode* DisabledNode = Provider->GetRootInteractionNodes()[0]->GetChildNodes()[0];
	bool bOpenUI = false;
	TestFalse(TEXT("组件关闭时不能直接执行功能"), Provider->ExecuteInteractionNode(DisabledNode, Receiver, bOpenUI));
	Provider->FeatureConfig.bEnableWarehouse = true;
	if (!TestTrue(TEXT("启用组件功能后重载"), Provider->LoadInteractionTreeAsset())) return false;
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("树与组件都启用时显示子选项"), Widget->GetDialogueOptionCount(), 1);

	// 根功能节点也必须遵循组件开关。
	Asset->Nodes = {Warehouse};
	Asset->Roots = {Warehouse->NodeId};
	Provider->FeatureConfig.bEnableWarehouse = false;
	if (!TestTrue(TEXT("重载根功能节点"), Provider->LoadInteractionTreeAsset())) return false;
	TestEqual(TEXT("组件关闭时入口列表不显示根功能"), EntranceWidget->GetEntranceOptionCount(), 0);
	Provider->FeatureConfig.bEnableWarehouse = true;
	if (!TestTrue(TEXT("启用根功能并重载"), Provider->LoadInteractionTreeAsset())) return false;
	TestEqual(TEXT("组件启用时根功能显示为入口"), EntranceWidget->GetEntranceOptionCount(), 1);

	// 即使两处都勾选，没有节点也不会凭空生成组件中的功能。
	Root->Children.Reset();
	Asset->Nodes = {Root};
	Asset->Roots = {Root->NodeId};
	if (!TestTrue(TEXT("加载没有功能节点的树"), Provider->LoadInteractionTreeAsset())) return false;
	TestTrue(TEXT("组件启用但树无节点时不创建功能"), Provider->GetInteractionFeatures().IsEmpty());
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("组件启用但树无节点时不显示子选项"), Widget->GetDialogueOptionCount(), 0);
	Provider->HandleInteractionRangeEndOverlap(Player);
	return true;
}

/** 验证自动交互范围对多碰撞体、多角色组件、重新绑定和生命周期的处理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionRangeTest,
	"LxARPG.Interaction.AutomaticRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用引擎实际重叠缓存及事件验证范围并集，不依赖物理帧推进。 */
bool FLxInteractionRangeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	AActor* Owner = World->SpawnActor<AActor>();
	ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建交互对象"), Owner) || !TestNotNull(TEXT("创建玩家"), Player)) return false;
	Player->GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	ULxPlayerInteractionModule* Receiver = Player->GetPlayerInteractionComponent();
	if (!TestNotNull(TEXT("玩家持有交互模块"), Receiver)) return false;
	Receiver->InitializeModule(Player->GetPlayerControlComponent());

	/** 创建互不接触的测试球体，后续显式注入引擎重叠记录，避免物理查询影响测试顺序。 */
	auto CreateSphere = [World](AActor* Actor, float X)
	{
		USphereComponent* Sphere = NewObject<USphereComponent>(Actor);
		Sphere->SetSphereRadius(20.0f);
		Sphere->SetWorldLocation(FVector(X, 0.0f, 0.0f));
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetCollisionResponseToAllChannels(ECR_Overlap);
		Sphere->SetGenerateOverlapEvents(true);
		Sphere->RegisterComponentWithWorld(World);
		return Sphere;
	};
	USphereComponent* RangeA = CreateSphere(Owner, 10000.0f);
	USphereComponent* RangeB = CreateSphere(Owner, 20000.0f);
	USphereComponent* PlayerBodyA = CreateSphere(Player, 30000.0f);
	USphereComponent* PlayerBodyB = CreateSphere(Player, 40000.0f);
	ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Owner);
	Provider->bCreateAssetInteractionRange = false;
	Provider->FeatureConfig.bEnableTradeContainer = true;
	ULxInteractionTreeAsset* DialogueAsset = NewObject<ULxInteractionTreeAsset>();
	DialogueAsset->Features.bEnableTradeContainer = true;
	ULxInteractionTreeNodeData* TradeData = NewObject<ULxInteractionTreeNodeData>(DialogueAsset);
	TradeData->NodeId = FGuid::NewGuid();
	TradeData->Type = ELxInteractionActionType::TradeContainer;
	TradeData->PromptText = FText::FromString(TEXT("购买药剂"));
	ULxInteractionTreeNodeData* EntranceData = NewObject<ULxInteractionTreeNodeData>(DialogueAsset);
	EntranceData->NodeId = FGuid::NewGuid();
	EntranceData->Type = ELxInteractionActionType::Entrance;
	EntranceData->PromptText = FText::FromString(TEXT("与商人交互"));
	EntranceData->NpcDialogueText = FText::FromString(TEXT("需要什么？"));
	EntranceData->Children.Add(TradeData->NodeId);
	DialogueAsset->Nodes = {EntranceData, TradeData};
	DialogueAsset->Roots.Add(EntranceData->NodeId);
	if (!TestTrue(TEXT("范围测试加载商人交互资产"), Provider->SetInteractionTreeAsset(DialogueAsset))) return false;
	Provider->RegisterComponentWithWorld(World);
	World->SetBegunPlay(true);
	Owner->DispatchBeginPlay();

	// 绑定时已重叠的角色应立即被纳入，空项和重复碰撞体不产生重复候选。
	const FOverlapInfo BodyA(PlayerBodyA);
	const FOverlapInfo BodyB(PlayerBodyB);
	RangeA->BeginComponentOverlap(BodyA, true);
	Provider->SetInteractionRangeColliders({RangeA, RangeA, nullptr, RangeB});
	TestTrue(TEXT("绑定时同步已在范围内的玩家"), Receiver->IsInteractableComponentInRange(Provider));
	Provider->SetInteractionRangeColliders({RangeA, RangeB});
	TestEqual(TEXT("重复设置不重复加入候选"), Receiver->GetInteractableQueue().Num(), 1);
	RangeB->BeginComponentOverlap(BodyA, true);
	RangeB->BeginComponentOverlap(BodyB, true);
	RangeA->EndComponentOverlap(BodyA);
	TestTrue(TEXT("离开一个碰撞体仍保留交互"), Receiver->IsInteractableComponentInRange(Provider));
	RangeB->EndComponentOverlap(BodyA);
	TestTrue(TEXT("角色另一碰撞组件仍重叠时保留交互"), Receiver->IsInteractableComponentInRange(Provider));
	RangeB->EndComponentOverlap(BodyB);
	TestFalse(TEXT("离开全部范围后移除交互"), Receiver->IsInteractableComponentInRange(Provider));

	// 替换或清空范围会移除旧监听和候选，重新绑定会同步已存在的重叠。
	RangeA->BeginComponentOverlap(BodyA, true);
	Provider->SetInteractionRangeColliders({RangeB});
	TestFalse(TEXT("替换列表移除只处于旧范围的玩家"), Receiver->IsInteractableComponentInRange(Provider));
	RangeA->EndComponentOverlap(BodyA);
	RangeA->BeginComponentOverlap(BodyA, true);
	TestFalse(TEXT("旧碰撞体已解除监听"), Receiver->IsInteractableComponentInRange(Provider));
	RangeB->BeginComponentOverlap(BodyA, true);
	TestTrue(TEXT("新碰撞体正常监听"), Receiver->IsInteractableComponentInRange(Provider));
	Provider->SetInteractionRangeColliders({});
	TestFalse(TEXT("空数组清理当前候选"), Receiver->IsInteractableComponentInRange(Provider));
	RangeB->EndComponentOverlap(BodyA);
	RangeB->BeginComponentOverlap(BodyA, true);
	TestFalse(TEXT("空数组解除全部监听"), Receiver->IsInteractableComponentInRange(Provider));

	// 资产中的任意根节点均可作为直接入口，交易配置自动生成对应功能模块。
	if (!TestEqual(TEXT("商人资产包含一个入口"), Provider->GetRootInteractionNodes().Num(), 1)) return false;
	ULxInteractionNode* EntranceNode = Provider->GetRootInteractionNodes()[0];
	if (!TestEqual(TEXT("商人入口包含一个交易子项"), EntranceNode->GetChildNodes().Num(), 1)) return false;
	ULxInteractionNode* TradeChildNode = EntranceNode->GetChildNodes()[0];
	TestNotNull(TEXT("交易子节点已创建功能模块"), TradeChildNode->GetInteractionFeature());
	TestTrue(TEXT("交易子节点类型有效"), TradeChildNode->ValidateNodeType());
	TestTrue(TEXT("交易子节点功能模块有效"), TradeChildNode->ValidateInteractionFeatureType());
	TestTrue(TEXT("玩家满足交易子节点通用限制"), TradeChildNode->CheckCommonRequirement(Receiver));
	ULxDialogueInteractionWidget* DialogueWidget = NewObject<ULxDialogueInteractionWidget>();
	DialogueWidget->SetPlayerInteractionComponent(Receiver);
	Provider->HandleInteractionRangeBeginOverlap(Player);
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("入口节点的交易子项显示在对话选项中"), DialogueWidget->GetDialogueOptionCount(), 1);
	FLxInteractionOption StaleEntranceOption;
	StaleEntranceOption.SourceInteractionComponent = Provider;
	StaleEntranceOption.InteractionNode = EntranceNode;
	StaleEntranceOption.InteractionType = ELxInteractionActionType::Entrance;

	ULxInteractionTreeAsset* DirectTradeAsset = NewObject<ULxInteractionTreeAsset>();
	DirectTradeAsset->Features.bEnableTradeContainer = true;
	ULxInteractionTreeNodeData* DirectTradeData = DuplicateObject<ULxInteractionTreeNodeData>(TradeData, DirectTradeAsset);
	DirectTradeAsset->Nodes.Add(DirectTradeData);
	DirectTradeAsset->Roots.Add(DirectTradeData->NodeId);
	if (!TestTrue(TEXT("切换为直接交易资产"), Provider->SetInteractionTreeAsset(DirectTradeAsset))) return false;
	TestEqual(TEXT("对话中切换资产会取消旧交互"), Receiver->GetInteractionPhase(), ELxPlayerInteractionPhase::None);
	TestEqual(TEXT("对话中切换资产会关闭对话窗口"), DialogueWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("旧资产选项即使来源组件仍在范围内也不能激活"), Receiver->ActivateInteractionOption(StaleEntranceOption));
	DialogueWidget->SetPlayerInteractionComponent(nullptr);
	ULxInteractionNode* DirectTradeNode = Provider->GetRootInteractionNodes()[0];
	Provider->HandleInteractionRangeBeginOverlap(Player);
	TestTrue(TEXT("手动进入函数保持可用"), Receiver->IsInteractableComponentInRange(Provider));
	TestTrue(TEXT("交易枚举节点自动成为有效功能节点"), DirectTradeNode->IsNodeInteractable(Receiver));
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("根交易节点可由入口按键直接打开功能界面"), Receiver->GetInteractionPhase(),
		ELxPlayerInteractionPhase::Function);
	TestFalse(TEXT("打开商城后交易节点不重复显示为入口"), DirectTradeNode->IsNodeInteractable(Receiver));
	TestTrue(TEXT("打开商城后服务端仍可处理购买和出售请求"),
		DirectTradeNode->CanProcessActiveInteractionRequest(Receiver));
	Provider->SetInteractionTreeAsset(nullptr);
	TestEqual(TEXT("打开功能界面时卸载资产会取消交互"), Receiver->GetInteractionPhase(), ELxPlayerInteractionPhase::None);
	TestFalse(TEXT("资产卸载后拒绝旧功能请求"), DirectTradeNode->CanProcessActiveInteractionRequest(Receiver));
	Provider->HandleInteractionRangeEndOverlap(Player);
	TestFalse(TEXT("手动离开函数保持可用"), Receiver->IsInteractableComponentInRange(Provider));

	// 注入已打开页面的持有状态，以真实返回入口验证清理事件，无需测试窗口或本地视口。
	ULxInteractionTreeAsset* PageAsset = DuplicateObject<ULxInteractionTreeAsset>(DialogueAsset, GetTransientPackage());
	PageAsset->Features.bEnableFunctionPage = true;
	Provider->FeatureConfig.bEnableFunctionPage = true;
	Provider->FeatureConfig.FunctionPageConfig.PageWidgetClass = ULxDialogueInteractionWidget::StaticClass();
	PageAsset->Nodes[1]->Type = ELxInteractionActionType::FunctionPage;
	if (!TestTrue(TEXT("加载功能页面资产"), Provider->SetInteractionTreeAsset(PageAsset))) return false;
	Provider->HandleInteractionRangeBeginOverlap(Player);
	Receiver->SelectEntranceOptionByIndex(0);
	ULxInteractionNode* PageRoot = Provider->GetRootInteractionNodes()[0];
	ULxInteractionNode* PageNode = PageRoot->GetChildNodes()[0];
	ULxFunctionPageInteractionComponent* PageFeature = CastChecked<ULxFunctionPageInteractionComponent>(PageNode->GetInteractionFeature());
	ULxInteractionUIManager* PageManager = NewObject<ULxInteractionUIManager>();
	PageManager->SetPlayerInteractionComponent(Receiver);
	ULxDialogueInteractionWidget* PageWidget = NewObject<ULxDialogueInteractionWidget>();
	PageWidget->SetVisibility(ESlateVisibility::Visible);
	FObjectPropertyBase* CurrentNodeProperty = FindFProperty<FObjectPropertyBase>(Receiver->GetClass(), TEXT("CurrentInteractionNode"));
	FObjectPropertyBase* PageWidgetProperty = FindFProperty<FObjectPropertyBase>(PageManager->GetClass(), TEXT("FunctionPageWidget"));
	FObjectPropertyBase* PageFeatureProperty = FindFProperty<FObjectPropertyBase>(PageManager->GetClass(), TEXT("FunctionPageFeature"));
	if (!TestNotNull(TEXT("当前交互节点反射属性"), CurrentNodeProperty)
		|| !TestNotNull(TEXT("当前功能页面反射属性"), PageWidgetProperty)
		|| !TestNotNull(TEXT("当前功能模块反射属性"), PageFeatureProperty)) return false;
	CurrentNodeProperty->SetObjectPropertyValue_InContainer(Receiver, PageNode);
	PageWidgetProperty->SetObjectPropertyValue_InContainer(PageManager, PageWidget);
	PageFeatureProperty->SetObjectPropertyValue_InContainer(PageManager, PageFeature);
	PageFeature->SetInteractionState(ELxInteractionDataState::Interacting);
	Receiver->BackToParentInteractionNode();
	TestNull(TEXT("返回父对话释放功能页面"), PageManager->GetActiveFunctionPageWidget());
	TestNull(TEXT("返回父对话解除页面模块引用"), PageManager->GetActiveFunctionPageFeature());
	TestEqual(TEXT("返回父对话隐藏原页面"), PageWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("返回父对话恢复功能可交互状态"), PageFeature->GetInteractionState(), ELxInteractionDataState::Interactable);
	TestEqual(TEXT("页面清理不取消父对话"), Receiver->GetInteractionPhase(), ELxPlayerInteractionPhase::Navigation);
	PageManager->SetPlayerInteractionComponent(nullptr);
	Provider->SetInteractionRangeColliders({RangeB});
	Provider->DestroyComponent();
	TestFalse(TEXT("销毁交互组件清理候选"), Receiver->IsInteractableComponentInRange(Provider));
	RangeB->EndComponentOverlap(BodyA);
	RangeB->BeginComponentOverlap(BodyA, true);
	TestEqual(TEXT("销毁后不再接收范围事件"), Receiver->GetInteractableQueue().Num(), 0);
	return true;
}

#endif
