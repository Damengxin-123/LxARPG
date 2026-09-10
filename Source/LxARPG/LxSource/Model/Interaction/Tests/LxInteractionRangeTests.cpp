#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNodeFunctionLibrary.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "UObject/UnrealType.h"

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

	// 任意根节点均可作为直接入口，通用节点创建函数也能按交易类型自动创建功能模块。
	FBoolProperty* EnableTradeProperty = FindFProperty<FBoolProperty>(Provider->GetClass(), TEXT("bEnableTradeContainer"));
	if (!TestNotNull(TEXT("可读取交易功能开关"), EnableTradeProperty)) return false;
	EnableTradeProperty->SetPropertyValue_InContainer(Provider, true);
	ULxInteractionNode* TradeChildNode = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		Provider, FText::FromString(TEXT("购买药剂")), FText(),
		ELxInteractionActionType::TradeContainer, FLxInteractionRequirement(), {});
	ULxInteractionNode* EntranceNode = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		Provider, FText::FromString(TEXT("与商人交互")), FText::FromString(TEXT("需要什么？")),
		ELxInteractionActionType::Entrance, FLxInteractionRequirement(), {TradeChildNode});
	Provider->BuildInteractionTree({EntranceNode});
	TestNotNull(TEXT("交易子节点已创建功能模块"), TradeChildNode->GetInteractionFeature());
	TestTrue(TEXT("交易子节点类型有效"), TradeChildNode->ValidateNodeType());
	TestTrue(TEXT("交易子节点功能模块有效"), TradeChildNode->ValidateInteractionFeatureType());
	TestTrue(TEXT("玩家满足交易子节点通用限制"), TradeChildNode->CheckCommonRequirement(Receiver));
	ULxDialogueInteractionWidget* DialogueWidget = NewObject<ULxDialogueInteractionWidget>();
	DialogueWidget->SetPlayerInteractionComponent(Receiver);
	Provider->HandleInteractionRangeBeginOverlap(Player);
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("入口节点的交易子项显示在对话选项中"), DialogueWidget->GetDialogueOptionCount(), 1);
	Provider->HandleInteractionRangeEndOverlap(Player);
	DialogueWidget->SetPlayerInteractionComponent(nullptr);

	ULxInteractionNode* DirectTradeNode = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		Provider, FText::FromString(TEXT("购买药剂")), FText(),
		ELxInteractionActionType::TradeContainer, FLxInteractionRequirement(), {});
	Provider->BuildInteractionTree({DirectTradeNode});
	Provider->HandleInteractionRangeBeginOverlap(Player);
	TestTrue(TEXT("手动进入函数保持可用"), Receiver->IsInteractableComponentInRange(Provider));
	TestTrue(TEXT("交易枚举节点自动成为有效功能节点"), DirectTradeNode->IsNodeInteractable(Receiver));
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("根交易节点可由入口按键直接打开功能界面"), Receiver->GetInteractionPhase(),
		ELxPlayerInteractionPhase::Function);
	TestFalse(TEXT("打开商城后交易节点不重复显示为入口"), DirectTradeNode->IsNodeInteractable(Receiver));
	TestTrue(TEXT("打开商城后服务端仍可处理购买和出售请求"),
		DirectTradeNode->CanProcessActiveInteractionRequest(Receiver));
	Provider->HandleInteractionRangeEndOverlap(Player);
	TestFalse(TEXT("手动离开函数保持可用"), Receiver->IsInteractableComponentInRange(Provider));
	Provider->SetInteractionRangeColliders({RangeB});
	Provider->DestroyComponent();
	TestFalse(TEXT("销毁交互组件清理候选"), Receiver->IsInteractableComponentInRange(Provider));
	RangeB->EndComponentOverlap(BodyA);
	RangeB->BeginComponentOverlap(BodyA, true);
	TestEqual(TEXT("销毁后不再接收范围事件"), Receiver->GetInteractableQueue().Num(), 0);
	return true;
}

#endif
