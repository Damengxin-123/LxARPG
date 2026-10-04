#include "LxSkillFlowTestUnits.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxElementAbnormalAttachSkillUnitActor.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

ALxSkillFlowTestProjectile::ALxSkillFlowTestProjectile()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试球体"));
	Shape->SetupAttachment(GetRootComponent()); Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestRay::ALxSkillFlowTestRay()
{
	auto* Shape = CreateDefaultSubobject<UCapsuleComponent>(TEXT("测试胶囊"));
	Shape->SetupAttachment(GetRootComponent()); Shape->SetCapsuleSize(10.f, 50.f);
	Shape->SetRelativeLocation(FVector(50, 0, 0)); Shape->SetRelativeRotation(FRotator(90, 0, 0));
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestGroundBounce::ALxSkillFlowTestGroundBounce()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestLob::ALxSkillFlowTestLob()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestDurationArea::ALxSkillFlowTestDurationArea()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestScalingArea::ALxSkillFlowTestScalingArea()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestMelee::ALxSkillFlowTestMelee()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestSingleRay::ALxSkillFlowTestSingleRay()
{
	auto* Shape = CreateDefaultSubobject<UCapsuleComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetCapsuleSize(10.f, 50.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestContinuousAttach::ALxSkillFlowTestContinuousAttach()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestPeriodicAttach::ALxSkillFlowTestPeriodicAttach()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestContinuousAura::ALxSkillFlowTestContinuousAura()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestPeriodicAura::ALxSkillFlowTestPeriodicAura()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestSpawnEntity::ALxSkillFlowTestSpawnEntity()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestBarrier::ALxSkillFlowTestBarrier()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestMarker::ALxSkillFlowTestMarker()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestSummonCreature::ALxSkillFlowTestSummonCreature()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestTrigger::ALxSkillFlowTestTrigger()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试碰撞"));
	Shape->SetupAttachment(GetRootComponent());
	Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

ALxSkillFlowTestArea::ALxSkillFlowTestArea()
{
	auto* Shape = CreateDefaultSubobject<USphereComponent>(TEXT("测试球体"));
	Shape->SetupAttachment(GetRootComponent()); Shape->SetSphereRadius(10.f);
	Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "EngineUtils.h"
#include "UObject/StrongObjectPtr.h"
#include "LxSkillFlowEdGraph.h"
#include "LxSkillFlowAssetEditor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillFlowExecution.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "Tests/AutomationCommon.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Toolkits/IToolkitHost.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Misc/ScopeExit.h"

namespace
{
	/** 建立与清理无地图依赖的运行时测试世界。 */
	struct FFlowWorld
	{
		/** 测试使用的世界。 */
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		/** 初始化世界并开启单元生命周期。 */
		FFlowWorld()
		{
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL()); World->BeginPlay();
		}
		/** 无论断言是否提前返回，都清理测试世界。 */
		~FFlowWorld() { World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
	};
	/** 创建带可执行默认参数的流程配置。 */
	ULxSkillFlowNode* AddNode(ULxSkillFlowAsset* Asset, ELxSkillFlowNodeKind Kind, ELxSkillFlowEvent Event = ELxSkillFlowEvent::Direct)
	{
		auto* Node = NewObject<ULxSkillFlowNode>(Asset);
		Node->Id = FGuid::NewGuid(); Node->Kind = Kind; Node->Event = Event;
		Node->ProjectileClass = ALxSkillFlowTestProjectile::StaticClass();
		Node->RayClass = ALxSkillFlowTestRay::StaticClass();
		Node->Projectile.ProjectileSpec.FlightSpeed = 10; Node->Projectile.ProjectileSpec.MaxFlightDistance = 30;
		if (Kind == ELxSkillFlowNodeKind::Ray) Node->Lifetime = ELxSkillFlowLifetime::Maintained;
		Asset->Nodes.Add(Node);
		return Node;
	}
	/** 取得当前世界中的独立技能流程实例。 */
	TArray<ALxSkillFlowExecution*> Flows(UWorld* World)
	{
		TArray<ALxSkillFlowExecution*> Result;
		for (TActorIterator<ALxSkillFlowExecution> It(World); It; ++It) Result.Add(*It);
		return Result;
	}
	/** 收集流程持有的存活单元，供生命周期断言。 */
	TArray<ALxSkillUnitActor*> Units(ALxSkillFlowExecution* Flow)
	{
		TArray<ALxSkillUnitActor*> Result;
		for (ULxSkillUnitGroup* Group : Flow->GetActiveGroups()) Result.Append(Group->GetSkillUnits());
		return Result;
	}
}

/** 技能流程缺少世界或初始化失败时必须返回失败，且不能留下蓄力占用。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowFailurePropagationTest, "LxARPG.SkillFlow.FailurePropagation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 使用真实流程初始化失败，检查失败状态传播、实例清理和修复后的重试。 */
bool FLxSkillFlowFailurePropagationTest::RunTest(const FString& Parameters)
{
	FFlowWorld TestWorld;
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get());
	Skill->FlowAsset = Asset;
	Asset->ReleaseType = ELxSkillReleaseType::ChargeRelease;
	AddExpectedError(TEXT("缺少有效世界"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("缺少世界的蓄力请求返回失败"), Skill->BeginSkillCharge());
	TestFalse(TEXT("失败请求不会进入可结束的蓄力状态"), Skill->TryBeginChargeSkillReleaseTiming());

	AActor* Caster = TestWorld.World->SpawnActor<AActor>();
	FLxSkillCastContext Context;
	Context.WorldContextObject = Caster;
	Context.CasterActor = Caster;
	Skill->PrepareSkillForCast(Context);
	AddExpectedError(TEXT("技能流程无法运行"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("无有效入口的流程不能开始蓄力"), Skill->BeginSkillCharge());
	TestFalse(TEXT("初始化失败不会留下蓄力占用"), Skill->TryBeginChargeSkillReleaseTiming());
	TestTrue(TEXT("初始化失败销毁流程实例"), Flows(TestWorld.World).IsEmpty());

	auto* Entry = AddNode(Asset, ELxSkillFlowNodeKind::Event, ELxSkillFlowEvent::ChargeEnd);
	auto* Projectile = AddNode(Asset, ELxSkillFlowNodeKind::Projectile);
	Entry->Next.Add(Projectile->Id);
	TestTrue(TEXT("修复配置后可立即重新蓄力"), Skill->BeginSkillCharge());
	TestTrue(TEXT("修复后可以正常结束蓄力"), Skill->TryBeginChargeSkillReleaseTiming());
	TestTrue(TEXT("有效蓄力结束事件返回成功"), Skill->ExecuteChargeSkillRelease());
	Skill->TryCancelSkillRelease();
	return true;
}

/** 动画等待期间流程变为无效时，失败必须清理角色和技能内部的释放状态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowAnimationFailureTest, "LxARPG.SkillFlow.AnimationFailureCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 通过真实数据中转动画通知触发流程失败，验证直接与持续技能都能解除占用。 */
bool FLxSkillFlowAnimationFailureTest::RunTest(const FString& Parameters)
{
	FFlowWorld TestWorld;
	auto* Character = TestWorld.World->SpawnActor<ALxBaseCharacter>();
	if (!TestNotNull(TEXT("创建技能失败测试角色"), Character)) return false;
	auto* Combat = Character->GetCharacterCombatComponent();
	auto* Transfer = Character->GetCharacterDataTransferComponent();
	auto* Behavior = Character->GetCharacterBehaviorControlComponent();
	Combat->BaseComponentInitialize();
	auto* Module = Combat->GetSkillCastModule();
	FLxCharacterMotionSignal LastMotion;
	const FDelegateHandle MotionHandle = Behavior->OnActionMotionSignalChanged.AddLambda(
		[&LastMotion](const FLxCharacterMotionSignal& Signal) { LastMotion = Signal; });
	ON_SCOPE_EXIT { Behavior->OnActionMotionSignalChanged.Remove(MotionHandle); };
	for (ELxSkillReleaseType Type : {ELxSkillReleaseType::DirectRelease, ELxSkillReleaseType::SustainedRelease})
	{
		TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>(Character));
		auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get());
		Skill->FlowAsset = Asset;
		Asset->ReleaseType = Type;
		const bool bSustained = Type == ELxSkillReleaseType::SustainedRelease;
		auto* Entry = AddNode(Asset, ELxSkillFlowNodeKind::Event,
			bSustained ? ELxSkillFlowEvent::SustainStart : ELxSkillFlowEvent::Direct);
		auto* Projectile = AddNode(Asset, ELxSkillFlowNodeKind::Projectile);
		Entry->Next.Add(Projectile->Id);
		const FLxSkillCastContext Context = Module->MakeSkillCastContext(Skill.Get());
		const bool bStarted = bSustained ? Module->StartSustainedRelease(Skill.Get(), Context)
			: Module->ReleaseSkillDirectly(Skill.Get(), Context);
		if (!TestTrue(TEXT("有效流程进入等待动画状态"), bStarted)
			|| !TestTrue(TEXT("动作信号携带释放编号"), LastMotion.CastId.IsValid())) return false;

		// 初始校验通过后破坏入口，复现实际执行时二次初始化失败。
		Entry->Next.Reset();
		FLxCharacterAnimationEvent Event;
		Event.bActionChannel = true;
		Event.CastId = LastMotion.CastId;
		Event.SkillId = LastMotion.SkillId;
		Event.NotifyName = FLxCharacterAnimationEvent::ReleaseName();
		AddExpectedError(TEXT("技能流程无法运行"), EAutomationExpectedErrorFlags::Contains, 1);
		Transfer->OnAnimationEvent.Broadcast(Event);
		TestTrue(TEXT("执行失败立即解除角色释放占用"), Module->IsSkillCastIdle());
		TestFalse(TEXT("执行失败清除持续运行状态"), Skill->IsSustainedReleaseActive());
		TestTrue(TEXT("执行失败清除技能内部释放占用"), Skill->IsReleaseCooldownReady());
		TestTrue(TEXT("执行失败不保留流程实例"), Flows(TestWorld.World).IsEmpty());

		Entry->Next.Add(Projectile->Id);
		TestTrue(TEXT("修复后角色可立即重新使用同一技能"), bSustained
			? Module->StartSustainedRelease(Skill.Get(), Context)
			: Module->ReleaseSkillDirectly(Skill.Get(), Context));
		Module->CancelCurrentSkillRelease();
	}
	return true;
}

/** 自主投射物不受取消、再次释放和释放者销毁影响，后续命中仍有自己的上下文。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowIndependentTest, "LxARPG.SkillFlow.IndependentLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 从技能公开入口验证释放隔离及旧命中分支。 */
bool FLxSkillFlowIndependentTest::RunTest(const FString& Parameters)
{
	FFlowWorld TestWorld;
	AActor* Caster = TestWorld.World->SpawnActor<AActor>();
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get()); Skill->FlowAsset = Asset;
	auto* Entry = AddNode(Asset, ELxSkillFlowNodeKind::Event);
	auto* Projectile = AddNode(Asset, ELxSkillFlowNodeKind::Projectile);
	auto* Child = AddNode(Asset, ELxSkillFlowNodeKind::Projectile);
	Child->SpawnLocation = ELxSkillUnitResultSpawnLocationType::HitLocation;
	Entry->Next.Add(Projectile->Id); Projectile->Hit.Add(Child->Id);
	FLxSkillCastContext Context; Context.WorldContextObject = Caster; Context.CasterActor = Caster;
	Context.bOverrideSpawnTransform = true; Context.SpawnTransform = FTransform(FVector(100, 0, 0));
	Skill->PrepareSkillForCast(Context);
	TestTrue(TEXT("直接释放通过原技能入口启动"), Skill->TryBeginDirectSkillReleaseTiming());
	Skill->ExecuteDirectSkillRelease(); Skill->CompleteSkillReleaseTiming();
	const auto FirstFlows = Flows(TestWorld.World);
	if (!TestEqual(TEXT("创建一个流程实例"), FirstFlows.Num(), 1)) return false;
	auto* First = FirstFlows[0]; First->Tick(0.f);
	auto FirstUnits = Units(First);
	if (!TestEqual(TEXT("创建第一发投射物"), FirstUnits.Num(), 1)) return false;
	auto* Fireball = FirstUnits[0];
	Skill->TryCancelSkillRelease();
	TestTrue(TEXT("取消角色释放不销毁已发射火球"), IsValid(Fireball) && Fireball->IsSkillUnitActive());
	Context.SpawnTransform = FTransform(FVector(900, 0, 0)); Skill->PrepareSkillForCast(Context);
	Skill->DispatchFlowEvent(ELxSkillFlowEvent::Direct);
	TestEqual(TEXT("再次释放创建独立实例"), Flows(TestWorld.World).Num(), 2);
	Caster->Destroy();
	FLxSkillUnitResult Hit; Hit.bSuccess = true; Hit.ResultType = ELxSkillUnitResultType::Hit;
	Hit.HitLocations.Add(FVector(400, 0, 0));
	Fireball->OnSkillUnitHit.Broadcast(Fireball, Hit);
	First->Tick(0.f);
	TestEqual(TEXT("释放者销毁后命中分支仍执行"), Units(First).Num(), 2);
	bool bChildAtHit = false;
	for (auto* Unit : Units(First)) bChildAtHit |= Unit != Fireball && Unit->GetActorLocation().Equals(FVector(400, 0, 0));
	TestTrue(TEXT("旧流程后续单元使用本次命中位置"), bChildAtHit);
	for (auto* Unit : Units(First)) Unit->StopSkillUnit();
	First->Tick(0.f);
	TestTrue(TEXT("所有自主单元结束后回收流程"), First->IsActorBeingDestroyed());
	return true;
}

/** 持续释放仅停止维持单元，并阻止排队的维持节点在松手后启动。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowMaintainedTest, "LxARPG.SkillFlow.MaintainedLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 检查混合单元、松手与排队事件之间的边界。 */
bool FLxSkillFlowMaintainedTest::RunTest(const FString& Parameters)
{
	FFlowWorld TestWorld;
	AActor* Caster = TestWorld.World->SpawnActor<AActor>();
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get()); Skill->FlowAsset = Asset;
	Asset->ReleaseType = ELxSkillReleaseType::SustainedRelease;
	auto* Entry = AddNode(Asset, ELxSkillFlowNodeKind::Event, ELxSkillFlowEvent::SustainStart);
	auto* Ray = AddNode(Asset, ELxSkillFlowNodeKind::Ray);
	auto* Projectile = AddNode(Asset, ELxSkillFlowNodeKind::Projectile);
	Entry->Next = {Ray->Id, Projectile->Id};
	FLxSkillCastContext Context; Context.CasterActor = Caster; Context.WorldContextObject = Caster;
	Skill->PrepareSkillForCast(Context);
	TestTrue(TEXT("资产释放类型在输入路由前可读取"), Skill->IsSustainedReleaseSkill());
	TestTrue(TEXT("持续释放入口成功"), Skill->TryBeginSustainedSkillReleaseTiming());
	Skill->ExecuteSustainedSkillRelease();
	const auto Running = Flows(TestWorld.World);
	if (!TestEqual(TEXT("创建持续流程"), Running.Num(), 1)) return false;
	auto* Flow = Running[0]; Flow->Tick(0.f);
	if (!TestEqual(TEXT("持续流程同时产生两类单元"), Units(Flow).Num(), 2)) return false;
	ALxSkillUnitActor* Beam = nullptr; ALxSkillUnitActor* Fireball = nullptr;
	for (auto* Unit : Units(Flow)) { if (Unit->IsA<ALxSkillFlowTestRay>()) Beam = Unit; else Fireball = Unit; }
	if (!Beam || !Fireball) return false;
	Skill->TryUpdateSustainedReleaseTransform(FTransform(FVector(500, 0, 0)));
	TestTrue(TEXT("瞄准更新只移动维持射线"), Beam->GetActorLocation().Equals(FVector(500, 0, 0)) && !Fireball->GetActorLocation().Equals(FVector(500, 0, 0)));
	TestTrue(TEXT("松手通过原技能入口正常结束"), Skill->TryStopSustainedRelease());
	TestFalse(TEXT("松手后射线停止"), IsValid(Beam) && Beam->IsSkillUnitActive());
	TestTrue(TEXT("同次生成的自主火球仍继续运行"), IsValid(Fireball) && Fireball->IsSkillUnitActive());
	Skill->DispatchFlowEvent(ELxSkillFlowEvent::SustainStart);
	Skill->DispatchFlowEvent(ELxSkillFlowEvent::ReleaseEnd);
	for (auto* Other : Flows(TestWorld.World)) if (Other != Flow)
	{
		Other->Tick(0.f);
		for (auto* Unit : Units(Other)) TestFalse(TEXT("松手后排队射线不能重新激活"), Unit->IsA<ALxSkillFlowTestRay>());
	}
	TestTrue(TEXT("下一次释放结束不影响上一发火球"), Fireball->IsSkillUnitActive());
	return true;
}

/** 验证蓄力开始、结束和取消分支的维持规则。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowChargeTest, "LxARPG.SkillFlow.ChargeLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 通过实际蓄力入口验证预备效果和发射单元的生命周期转换。 */
bool FLxSkillFlowChargeTest::RunTest(const FString& Parameters)
{
	FFlowWorld TestWorld;
	AActor* Caster = TestWorld.World->SpawnActor<AActor>();
	TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
	auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get()); Skill->FlowAsset = Asset;
	Asset->ReleaseType = ELxSkillReleaseType::ChargeRelease;
	auto* Start = AddNode(Asset, ELxSkillFlowNodeKind::Event, ELxSkillFlowEvent::ChargeStart);
	auto* End = AddNode(Asset, ELxSkillFlowNodeKind::Event, ELxSkillFlowEvent::ChargeEnd);
	auto* Ray = AddNode(Asset, ELxSkillFlowNodeKind::Ray);
	auto* Projectile = AddNode(Asset, ELxSkillFlowNodeKind::Projectile);
	Start->Next.Add(Ray->Id); End->Next.Add(Projectile->Id);
	FLxSkillCastContext Context; Context.CasterActor = Caster; Context.WorldContextObject = Caster;
	Context.bOverrideSpawnTransform = true;
	Skill->PrepareSkillForCast(Context); Skill->BeginSkillCharge();
	const auto Running = Flows(TestWorld.World);
	if (!TestEqual(TEXT("开始蓄力创建流程"), Running.Num(), 1)) return false;
	auto* Flow = Running[0]; Flow->Tick(0.f);
	if (!TestEqual(TEXT("蓄力阶段运行维持效果"), Units(Flow).Num(), 1)) return false;
	auto* Beam = Units(Flow)[0];
	Context.SpawnTransform = FTransform(FVector(800, 0, 0)); Skill->PrepareSkillForCast(Context);
	TestTrue(TEXT("结束蓄力允许进入释放阶段"), Skill->TryBeginChargeSkillReleaseTiming());
	Skill->ExecuteChargeSkillRelease(); Flow->Tick(0.f);
	TestFalse(TEXT("结束蓄力停止蓄力维持效果"), IsValid(Beam) && Beam->IsSkillUnitActive());
	if (!TestEqual(TEXT("结束蓄力仅剩发射火球"), Units(Flow).Num(), 1)) return false;
	TestTrue(TEXT("蓄力发射使用结束时的位置"), Units(Flow)[0]->GetActorLocation().Equals(FVector(800, 0, 0)));
	Skill->TryCancelSkillRelease();
	TestTrue(TEXT("发射后取消不能收回火球"), Units(Flow)[0]->IsSkillUnitActive());
	return true;
}

/** 验证技能物品无需配套技能蓝图，延迟创建并复用释放状态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowItemTest, "LxARPG.SkillFlow.ItemBinding", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 通过真实释放模块检查物品配置、三种输入路由和运行状态保留。 */
bool FLxSkillFlowItemTest::RunTest(const FString& Parameters)
{
	FFlowWorld TestWorld;
	AActor* Caster = TestWorld.World->SpawnActor<AActor>();
	auto* Combat = NewObject<ULxCharacterCombatComponent>(Caster);
	Combat->RegisterComponent(); Combat->BaseComponentInitialize();
	auto* Module = Combat->GetSkillCastModule();
	for (ELxSkillReleaseType Type : {ELxSkillReleaseType::DirectRelease, ELxSkillReleaseType::ChargeRelease, ELxSkillReleaseType::SustainedRelease})
	{
		TStrongObjectPtr<ULxSkillFlowTestItem> Item(NewObject<ULxSkillFlowTestItem>(Caster));
		auto* Asset = NewObject<ULxSkillFlowAsset>(Item.Get());
		Asset->ReleaseType = Type;
		Asset->AnimationMotionType = ELxCharacterMotionType::RangedAttack;
		const ELxSkillFlowEvent Event = Type == ELxSkillReleaseType::DirectRelease ? ELxSkillFlowEvent::Direct
			: Type == ELxSkillReleaseType::ChargeRelease ? ELxSkillFlowEvent::ChargeEnd : ELxSkillFlowEvent::SustainStart;
		auto* Entry = AddNode(Asset, ELxSkillFlowNodeKind::Event, Event);
		auto* Unit = AddNode(Asset, Type == ELxSkillReleaseType::SustainedRelease ? ELxSkillFlowNodeKind::Ray : ELxSkillFlowNodeKind::Projectile);
		Entry->Next.Add(Unit->Id);
		FLxSkillItemInformation Information; Information.SkillFlow = Asset;
		Item->Configure(Information);
		TestNull(TEXT("加载物品配置不提前创建技能实体"), Item->GetSkillObject());
		const FLxSkillCastContext Context = Module->MakeSkillCastContext(Item.Get());
		const bool bStarted = Type == ELxSkillReleaseType::DirectRelease
			? Module->ReleaseSkillItemDirectly(Item.Get(), Context) : Module->StartUseSkillItem(Item.Get(), Context);
		if (!TestTrue(TEXT("只有流程资产也能通过物品入口开始释放"), bStarted)) return false;
		auto* Skill = Item->GetSkillObject();
		if (!TestNotNull(TEXT("使用时自动创建技能实体"), Skill)) return false;
		TestTrue(TEXT("运行对象为通用技能类，无需技能蓝图"), Skill->GetClass() == ULxSkill::StaticClass());
		TestEqual(TEXT("释放方式来自流程"), Skill->GetSkillReleaseType(), Type);
		TestEqual(TEXT("动作类型来自流程"), Skill->AnimationMotionType, Asset->AnimationMotionType);
		TestEqual(TEXT("技能身份仍来自所属物品"), Skill->GetSkillIDTag(), Information.ItemIDTag);
		TestFalse(TEXT("释放状态占用期间不能重复使用物品"), Type == ELxSkillReleaseType::DirectRelease
			? Module->ReleaseSkillItemDirectly(Item.Get(), Context) : Module->StartUseSkillItem(Item.Get(), Context));
		if (Type == ELxSkillReleaseType::ChargeRelease)
		{
			TestTrue(TEXT("松手使用同一运行对象结束蓄力"), Module->EndUseSkillItem(Item.Get(), Context));
			Skill->ExecuteChargeSkillRelease();
		}
		else if (Type == ELxSkillReleaseType::SustainedRelease) Skill->ExecuteSustainedSkillRelease();
		else Skill->ExecuteDirectSkillRelease();
		// 此测试模拟动画释放点调用；真实动画通知链由 Animation.SkillEvents 回归覆盖。
		const auto Running = Flows(TestWorld.World);
		if (!TestEqual(TEXT("本次物品释放创建一个独立流程"), Running.Num(), 1)) return false;
		auto* Flow = Running[0]; Flow->Tick(0.f);
		TestEqual(TEXT("运行流程生成子单元"), Units(Flow).Num(), 1);
		Skill->CompleteSkillReleaseTiming();
		if (Type == ELxSkillReleaseType::SustainedRelease)
		{
			TestTrue(TEXT("松手经物品入口停止持续释放"), Module->EndUseSkillItem(Item.Get(), Context));
			TestEqual(TEXT("松手后维持单元已结束"), Units(Flow).Num(), 0);
		}
		else Module->CancelCurrentSkillRelease();
		TestTrue(TEXT("后续取得运行对象不重新创建"), Item->GetOrCreateSkillObject() == Skill);
		TestTrue(TEXT("释放占用解除后物品可立即再次使用"), Type == ELxSkillReleaseType::DirectRelease
			? Module->ReleaseSkillItemDirectly(Item.Get(), Context) : Module->StartUseSkillItem(Item.Get(), Context));
		Module->CancelCurrentSkillRelease();
		for (auto* ActiveUnit : Units(Flow)) ActiveUnit->StopSkillUnit();
		for (auto* ActiveFlow : Flows(TestWorld.World)) ActiveFlow->Tick(0.f);
	}
	TStrongObjectPtr<ULxSkillFlowTestItem> Legacy(NewObject<ULxSkillFlowTestItem>(Caster));
	FLxSkillItemInformation LegacyInformation; LegacyInformation.SkillClass = ULxSkill::StaticClass();
	Legacy->Configure(LegacyInformation);
	TestNull(TEXT("旧技能也延迟创建"), Legacy->GetSkillObject());
	TestNull(TEXT("旧技能类型不再创建运行对象，必须配置流程"), Legacy->GetOrCreateSkillObject());
	return true;
}

/** 验证正式技能表已完成迁移，并通过真实子单元蓝图运行每种已实现技能的入口。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowMigratedAssetsTest, "LxARPG.SkillFlow.MigratedAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 加载磁盘中的正式物品与流程，检查延迟创建、缓存和真实蓝图生成。 */
bool FLxSkillFlowMigratedAssetsTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = LoadObject<UDataTable>(nullptr,TEXT("/Game/项目内容/数据资产/数据表格/物品信息/技能/技能数据表.技能数据表"));
	if (!TestNotNull(TEXT("正式技能表存在"),Table)) return false;
	TSet<ULxSkillFlowAsset*> Checked;
	int32 Bound = 0, Empty = 0;
	for (const auto& Pair : Table->GetRowMap())
	{
		const auto* Row = reinterpret_cast<const FLxSkillItemInformation*>(Pair.Value);
		TestNull(TEXT("正式物品不再引用技能蓝图"),Row->SkillClass.Get());
		if (!Row->SkillFlow) { ++Empty; continue; }
		++Bound;
		if (Checked.Contains(Row->SkillFlow)) continue;
		Checked.Add(Row->SkillFlow);
		if (!TestNotNull(TEXT("重启后正式流程的编辑图完整加载"),Row->SkillFlow->EditorGraph.Get())) return false;
		FFlowWorld TestWorld;
		auto* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
		Caster->SetActorEnableCollision(false);
		TStrongObjectPtr<ULxSkillFlowTestItem> Item(NewObject<ULxSkillFlowTestItem>(Caster));
		Item->Configure(*Row);
		TestNull(TEXT("加载正式物品不提前创建运行对象"),Item->GetSkillObject());
		ULxSkill* Skill = Item->GetOrCreateSkillObject();
		if (!TestNotNull(TEXT("流程物品能够创建运行对象"),Skill)) return false;
		TestTrue(TEXT("正式技能使用统一原生运行对象"),Skill->GetClass()==ULxSkill::StaticClass());
		TestTrue(TEXT("再次获取复用缓存"),Item->GetOrCreateSkillObject()==Skill);
		TArray<UClass*> CreatedTypes;
		const FDelegateHandle SpawnHandle = TestWorld.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
		{
			if (Actor->IsA<ALxSkillUnitActor>()) CreatedTypes.Add(Actor->GetClass());
		}));
		ON_SCOPE_EXIT { TestWorld.World->RemoveOnActorSpawnedHandler(SpawnHandle); };
		FLxSkillCastContext Context; Context.CasterActor=Caster; Context.WorldContextObject=Caster; Context.SourceObject=Item.Get();
		Skill->PrepareSkillForCast(Context);
		const bool bSustained = Skill->IsSustainedReleaseSkill();
		if (!TestTrue(TEXT("正式流程可开始释放"),bSustained ? Skill->TryBeginSustainedSkillReleaseTiming() : Skill->TryBeginDirectSkillReleaseTiming())) return false;
		if (bSustained) Skill->ExecuteSustainedSkillRelease(); else Skill->ExecuteDirectSkillRelease();
		const auto Running = Flows(TestWorld.World);
		if (!TestEqual(TEXT("只创建本次执行实例"),Running.Num(),1)) return false;
		auto* Flow = Running[0]; Flow->Tick(0.f);
		for (ALxSkillUnitActor* Unit : Units(Flow))
		{
			const auto* Config = Row->SkillFlow->Nodes.FindByPredicate([&](const ULxSkillFlowNode* N){ return N->GetSkillUnitClass()==Unit->GetClass(); });
			if (!Config || !(*Config)->bOverrideTargetRules) continue;
			const auto* Filter = FindFProperty<FStructProperty>(ULxSkillDetectionComponent::StaticClass(),TEXT("TargetFilterSpec"));
			const auto* Limit = FindFProperty<FStructProperty>(ULxSkillTriggerComponent::StaticClass(),TEXT("HitLimitSpec"));
			const auto* SpecProperty = FindFProperty<FStructProperty>(ALxSkillUnitActor::StaticClass(),TEXT("SkillUnitSpec"));
			const auto* Spec = SpecProperty->ContainerPtrToValuePtr<FLxSkillUnitSpec>(Unit);
			TestTrue(TEXT("旧命中限制实际传入单元运行参数"),FLxSkillHitLimitSpec::StaticStruct()->CompareScriptStruct(&Spec->HitLimitSpec,&(*Config)->HitLimit,PPF_None));
			// 投射物与射线可自行处理命中限制，不要求每个蓝图都额外挂触发组件。
			if (auto* Detection = Unit->GetSkillDetectionComponent())
				TestTrue(TEXT("旧筛选规则实际传入检测组件"),Filter->Identical(Filter->ContainerPtrToValuePtr<void>(Detection),&(*Config)->TargetFilter,PPF_None));
			if (auto* Trigger = Unit->GetSkillTriggerComponent())
				TestTrue(TEXT("旧命中限制实际传入触发组件"),Limit->Identical(Limit->ContainerPtrToValuePtr<void>(Trigger),&(*Config)->HitLimit,PPF_None));
		}
		for (const ULxSkillFlowNode* Entry : Row->SkillFlow->Nodes)
		{
			if (Entry->Kind!=ELxSkillFlowNodeKind::Event) continue;
			for (const FGuid& Id : Entry->Next)
			{
				const auto* Root = Row->SkillFlow->Nodes.FindByPredicate([&](const ULxSkillFlowNode* Node){ return Node->Id==Id; });
				TestTrue(Row->SkillFlow->GetName()+TEXT("入口创建了配置的子单元"),Root && CreatedTypes.Contains((*Root)->GetSkillUnitClass()));
			}
		}
		// 用真实火球蓝图的命中事件验证后续爆炸及词条来源，不依赖编辑器鼠标操作。
		if (Row->SkillFlow->GetName()==TEXT("火球术"))
		{
			auto* Target=TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(5000,0,0),FRotator::ZeroRotator);
			const auto Groups=Flow->GetActiveGroups();
			if (!TestEqual(TEXT("火球首段只有投射物"),Groups.Num(),1)) return false;
			FLxSkillUnitResult Hit; Hit.bSuccess=true; Hit.ResultType=ELxSkillUnitResultType::Hit;
			Hit.SourceUnit=Groups[0]->GetSkillUnits()[0]; Hit.HitTargets.Add(Target); Hit.HitTargetLocations.Add(Target->GetActorLocation()); Hit.HitLocations.Add(Target->GetActorLocation());
			Groups[0]->OnSkillUnitGroupHit.Broadcast(Groups[0],Hit); Flow->Tick(0.f);
			TestTrue(TEXT("真实火球命中创建缩放爆炸蓝图"),CreatedTypes.Contains(Row->SkillFlow->Nodes[2]->GetSkillUnitClass()));
		}
		if (bSustained)
		{
			Skill->TryStopSustainedRelease();
			for (auto* Unit : Units(Flow)) TestFalse(TEXT("松手后没有持续射线保持激活"),Unit->IsA<ALxContinuousRaySkillUnitActor>() && Unit->IsSkillUnitActive());
		}
	}
	TestEqual(TEXT("正式表已绑定十五行"),Bound,15);
	TestEqual(TEXT("十二种技能都经过真实蓝图创建检查"),Checked.Num(),12);
	TestEqual(TEXT("九个未实现占位不被凭空补成其他技能"),Empty,9);
	return true;
}

/** 验证菜单中的全部具体单元都能从图配置生成正确实体，依附还需验证目标传递与持续效果清理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowAllUnitsTest, "LxARPG.SkillFlow.AllUnitNodes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 使用带碰撞的原生测试子类逐一运行所有创建分支，避免依赖美术蓝图。 */
bool FLxSkillFlowAllUnitsTest::RunTest(const FString& Parameters)
{
	/** 单元类型与用户在面板中选择的具体类。 */
	struct FUnitCase
	{
		/** 对应菜单项。 */ ELxSkillFlowNodeKind Kind;
		/** 该节点的类型选择属性。 */ FName Property;
		/** 用于验证创建结果的测试子类。 */ UClass* Class;
	};
	const FUnitCase Cases[] = {
		{ELxSkillFlowNodeKind::Projectile, TEXT("ProjectileClass"), ALxSkillFlowTestProjectile::StaticClass()},
		{ELxSkillFlowNodeKind::Area, TEXT("AreaClass"), ALxSkillFlowTestArea::StaticClass()},
		{ELxSkillFlowNodeKind::Ray, TEXT("RayClass"), ALxSkillFlowTestRay::StaticClass()},
		{ELxSkillFlowNodeKind::GroundBounce, TEXT("GroundBounceClass"), ALxSkillFlowTestGroundBounce::StaticClass()},
		{ELxSkillFlowNodeKind::Lob, TEXT("LobClass"), ALxSkillFlowTestLob::StaticClass()},
		{ELxSkillFlowNodeKind::DurationArea, TEXT("DurationAreaClass"), ALxSkillFlowTestDurationArea::StaticClass()},
		{ELxSkillFlowNodeKind::ScalingArea, TEXT("ScalingAreaClass"), ALxSkillFlowTestScalingArea::StaticClass()},
		{ELxSkillFlowNodeKind::Melee, TEXT("MeleeClass"), ALxSkillFlowTestMelee::StaticClass()},
		{ELxSkillFlowNodeKind::SingleRay, TEXT("SingleRayClass"), ALxSkillFlowTestSingleRay::StaticClass()},
		{ELxSkillFlowNodeKind::ContinuousAttach, TEXT("ContinuousAttachClass"), ALxSkillFlowTestContinuousAttach::StaticClass()},
		{ELxSkillFlowNodeKind::PeriodicAttach, TEXT("PeriodicAttachClass"), ALxSkillFlowTestPeriodicAttach::StaticClass()},
		{ELxSkillFlowNodeKind::ContinuousAura, TEXT("ContinuousAuraClass"), ALxSkillFlowTestContinuousAura::StaticClass()},
		{ELxSkillFlowNodeKind::PeriodicAura, TEXT("PeriodicAuraClass"), ALxSkillFlowTestPeriodicAura::StaticClass()},
		{ELxSkillFlowNodeKind::SpawnEntity, TEXT("SpawnEntityClass"), ALxSkillFlowTestSpawnEntity::StaticClass()},
		{ELxSkillFlowNodeKind::Barrier, TEXT("BarrierClass"), ALxSkillFlowTestBarrier::StaticClass()},
		{ELxSkillFlowNodeKind::Marker, TEXT("MarkerClass"), ALxSkillFlowTestMarker::StaticClass()},
		{ELxSkillFlowNodeKind::SummonCreature, TEXT("SummonCreatureClass"), ALxSkillFlowTestSummonCreature::StaticClass()},
		{ELxSkillFlowNodeKind::Trigger, TEXT("TriggerClass"), ALxSkillFlowTestTrigger::StaticClass()},
		{ELxSkillFlowNodeKind::ElementAbnormalAttach, TEXT("ElementAbnormalAttachClass"), ALxElementAbnormalAttachSkillUnitActor::StaticClass()}
	};
	TestEqual(TEXT("覆盖全部十九种单元类型"), static_cast<int32>(UE_ARRAY_COUNT(Cases)), 19);
	TestEqual(TEXT("枚举中没有漏测的具体类型"), StaticEnum<ELxSkillFlowNodeKind>()->NumEnums() - 2, static_cast<int32>(UE_ARRAY_COUNT(Cases)));
	for (const FUnitCase& Case : Cases)
	{
		FFlowWorld TestWorld;
		TStrongObjectPtr<ALxSkillUnitActor> CreatedUnit;
		const FDelegateHandle SpawnHandle = TestWorld.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
		{
			if (Actor->GetClass() == Case.Class) CreatedUnit.Reset(CastChecked<ALxSkillUnitActor>(Actor));
		}));
		ON_SCOPE_EXIT { TestWorld.World->RemoveOnActorSpawnedHandler(SpawnHandle); };
		auto* Caster = TestWorld.World->SpawnActor<ALxBaseCharacter>();
		auto* Target = TestWorld.World->SpawnActor<ALxBaseCharacter>(FVector(5000, 0, 0), FRotator::ZeroRotator);
		TStrongObjectPtr<ULxSkill> Skill(NewObject<ULxSkill>());
		auto* Asset = NewObject<ULxSkillFlowAsset>(Skill.Get()); Skill->FlowAsset = Asset;
		Asset->ReleaseType = ELxSkillReleaseType::SustainedRelease;
		auto* Graph = NewObject<ULxSkillFlowEdGraph>(Asset); Asset->EditorGraph = Graph;
		Graph->Schema = ULxSkillFlowEdGraphSchema::StaticClass();
		FGraphContextMenuBuilder Menu(Graph);
		Graph->GetSchema()->GetGraphContextActions(Menu);
		TestEqual(TEXT("菜单包含全部单元与六个角色事件"), Menu.GetNumActions(), StaticEnum<ELxSkillFlowNodeKind>()->NumEnums() - 2 + StaticEnum<ELxSkillFlowEvent>()->NumEnums() - 1);
		FLxSkillFlowNewNodeAction EventAction; EventAction.Event = ELxSkillFlowEvent::SustainStart;
		auto* Entry = CastChecked<ULxSkillFlowEdGraphNode>(EventAction.PerformAction(Graph, nullptr, FVector2f::ZeroVector));
		FLxSkillFlowNewNodeAction UnitAction; UnitAction.Kind = Case.Kind;
		auto* UnitNode = CastChecked<ULxSkillFlowEdGraphNode>(UnitAction.PerformAction(Graph, nullptr, FVector2f(350, 0)));
		ULxSkillFlowNode* Node = UnitNode->Data;
		auto* ClassProperty = FindFProperty<FClassProperty>(Node->GetClass(), Case.Property);
		if (!TestNotNull(TEXT("节点提供相应子类选择器"), ClassProperty)) return false;
		ClassProperty->SetObjectPropertyValue_InContainer(Node, Case.Class);
		TestTrue(TEXT("节点名称读取当前选择的具体单元"), Node->GetSkillUnitClass().Get() == Case.Class && !Node->GetSkillUnitDisplayName().IsEmpty());
		const bool bAttach = Case.Kind == ELxSkillFlowNodeKind::ContinuousAttach || Case.Kind == ELxSkillFlowNodeKind::PeriodicAttach
			|| Case.Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach;
		if (Case.Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach)
		{
			Caster->InitialCharacterInformation(); Target->InitialCharacterInformation();
			Node->ElementAbnormalAttach.AbnormalSpec.StateTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("角色状态.元素异常状态.依附验证")));
		}
		ULxSkillFlowEdGraphNode* Source = Entry;
		if (bAttach)
		{
			FLxSkillFlowNewNodeAction HitAction; HitAction.Kind = ELxSkillFlowNodeKind::Projectile;
			Source = CastChecked<ULxSkillFlowEdGraphNode>(HitAction.PerformAction(Graph, Entry->Pins[0], FVector2f(180, 0)));
			Source->Data->ProjectileClass = ALxSkillFlowTestProjectile::StaticClass();
		}
		TestTrue(TEXT("创建菜单节点并连接前置结果"), Graph->GetSchema()->TryCreateConnection(
			Source->FindPin(bAttach ? TEXT("命中") : TEXT("执行")), UnitNode->FindPin(TEXT("创建"))));
		int32 PersistentHits = 0, RemovedEffects = 0;
		if (Case.Kind == ELxSkillFlowNodeKind::ContinuousAttach)
		{
			Asset->EntryPackages.AddDefaulted_GetRef().EntryQuotes.AddDefaulted();
			Node->EntryPackageIndex = 0;
			Skill->OnPersistentSkillHitEntriesReady.AddLambda([&](ULxSkill*, ALxSkillUnitActor*, const TArray<FLxSkillEntryPackage>&, const TArray<AActor*>&) { ++PersistentHits; });
			Skill->OnSkillEffectsRemoved.AddLambda([&](ULxSkill*, ALxSkillUnitActor*, const TArray<AActor*>&) { ++RemovedEffects; });
		}
		FText Error;
		if (!TestTrue(*FString::Printf(TEXT("节点 %s 配置有效：%s"), *Case.Property.ToString(), *Error.ToString()), Asset->Validate(Error))) return false;
		FLxSkillCastContext Context; Context.WorldContextObject = Caster; Context.CasterActor = Caster;
		Skill->PrepareSkillForCast(Context);
		Skill->TryBeginSustainedSkillReleaseTiming(); Skill->ExecuteSustainedSkillRelease();
		const auto Running = Flows(TestWorld.World);
		if (!TestEqual(TEXT("建立独立流程"), Running.Num(), 1)) return false;
		auto* Flow = Running[0]; Flow->Tick(0.f);
		if (bAttach)
		{
			const auto Sources = Units(Flow);
			if (!TestEqual(TEXT("依附效果等待前置命中"), Sources.Num(), 1)) return false;
			FLxSkillUnitResult Hit; Hit.bSuccess = true; Hit.ResultType = ELxSkillUnitResultType::Hit;
			Hit.SourceUnit = Sources[0]; Hit.HitTargets.Add(Target); Hit.HitLocations.Add(Target->GetActorLocation());
			Sources[0]->OnSkillUnitHit.Broadcast(Sources[0], Hit); Flow->Tick(0.f);
		}
		// 直接范围和单次射线可能在激活时立即完成，使用创建回调检查其实际类型。
		ALxSkillUnitActor* Created = CreatedUnit.Get();
		if (!TestNotNull(*FString::Printf(TEXT("%s 实际生成所选子单元"), *Case.Property.ToString()), Created)) return false;
		if (bAttach) TestTrue(TEXT("依附效果已挂接前置命中目标"), Created->GetAttachParentActor() == Target);
		if (Case.Kind == ELxSkillFlowNodeKind::ContinuousAttach) TestEqual(TEXT("持续词条投递委托传递到独立上下文"), PersistentHits, 1);
		Skill->TryStopSustainedRelease();
		if (Case.Kind == ELxSkillFlowNodeKind::Melee || Case.Kind == ELxSkillFlowNodeKind::Ray)
			TestFalse(TEXT("停止释放结束维持单元"), IsValid(Created) && Created->IsSkillUnitActive());
		// 模拟各单元自行结束；单次射线则模拟其表现保留时间结束。
		for (auto* Unit : Units(Flow))
		{
			Unit->StopSkillUnit();
			if (IsValid(Unit) && !Unit->IsActorBeingDestroyed()) Unit->Destroy();
		}
		Flow->Tick(0.f);
		if (Case.Kind == ELxSkillFlowNodeKind::ContinuousAttach) TestEqual(TEXT("依附结束后持续效果正确撤销"), RemovedEffects, 1);
		TestTrue(TEXT("所有单元结束后回收流程"), Flow->IsActorBeingDestroyed());
	}
	return true;
}

/** 验证图编译、循环校验和真实编辑器面板构建。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSkillFlowGraphTest, "LxARPG.SkillFlow.GraphEditor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
/** 构建真实编辑图，并可选渲染编辑器作为界面检查。 */
bool FLxSkillFlowGraphTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULxSkillFlowAsset> Asset(NewObject<ULxSkillFlowAsset>());
	auto* Graph = NewObject<ULxSkillFlowEdGraph>(Asset.Get()); Asset->EditorGraph = Graph;
	Graph->Schema = ULxSkillFlowEdGraphSchema::StaticClass(); Graph->EnsureEntry();
	auto* Entry = CastChecked<ULxSkillFlowEdGraphNode>(Graph->Nodes[0]);
	FLxSkillFlowNewNodeAction Action; Action.Kind = ELxSkillFlowNodeKind::Projectile;
	auto* Projectile = CastChecked<ULxSkillFlowEdGraphNode>(Action.PerformAction(Graph, Entry->Pins[0], FVector2f(350, 0)));
	auto* Child = CastChecked<ULxSkillFlowEdGraphNode>(Action.PerformAction(Graph, Projectile->FindPin(TEXT("命中")), FVector2f(700, 0)));
	FText Error;
	TestTrue(TEXT("画布连接编译成有效运行数据"), Asset->Validate(Error));
	TestEqual(TEXT("事件连接正确"), Entry->Data->Next[0], Projectile->Data->Id);
	TestEqual(TEXT("命中连接正确"), Projectile->Data->Hit[0], Child->Data->Id);
	TestFalse(TEXT("编辑器拒绝循环连接"), Graph->GetSchema()->TryCreateConnection(Child->FindPin(TEXT("命中")), Projectile->FindPin(TEXT("创建"))));
	Child->Data->Hit.Add(Projectile->Data->Id);
	TestFalse(TEXT("运行时也拒绝绕过编辑器写入的循环"), Asset->Validate(Error));
	Graph->SynchronizeAsset();
	auto* Copy = DuplicateObject<ULxSkillFlowAsset>(Asset.Get(), GetTransientPackage());
	CastChecked<ULxSkillFlowEdGraph>(Copy->EditorGraph)->SynchronizeAsset();
	TestTrue(TEXT("复制后的图与运行数据一致"), Copy->Validate(Error));
	TestNotEqual(TEXT("复制资产拥有独立节点"), Copy->Nodes[0].Get(), Asset->Nodes[0].Get());
	Asset->ReleaseType = ELxSkillReleaseType::ChargeRelease;
	Entry->Data->Event = ELxSkillFlowEvent::ChargeEnd;
	Child->Data->Lifetime = ELxSkillFlowLifetime::Maintained;
	TestFalse(TEXT("结束蓄力分支不能产生无人维持的单元"), Asset->Validate(Error));
	if (FParse::Param(FCommandLine::Get(), TEXT("SmokeSkillFlowEditor")) && FApp::CanEverRender())
	{
		FString PreviewPath = TEXT("/Game/项目内容/测试/技能流程/喷射流程.喷射流程");
		FParse::Value(FCommandLine::Get(),TEXT("SkillFlowPreview="),PreviewPath);
		auto* Demo = LoadObject<ULxSkillFlowAsset>(nullptr, *PreviewPath);
		if (!TestNotNull(TEXT("打开已保存的技能流程"), Demo)) return false;
		TSharedRef<FLxSkillFlowAssetEditor> Editor = MakeShared<FLxSkillFlowAssetEditor>();
		Editor->Init(EToolkitMode::Standalone, nullptr, Demo);
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Editor]()
		{
			TArray<FColor> Pixels; FIntVector Size;
			if (FSlateApplication::Get().TakeScreenshot(Editor->GetToolkitHost()->GetParentWidget(), Pixels, Size))
			{
				TArray64<uint8> Png;
				FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
				FFileHelper::SaveArrayToFile(Png, *(FPaths::ProjectSavedDir() / TEXT("Tests/技能流程编辑器.png")));
			}
			Editor->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed);
		}, 2.0f));
	}
	return true;
}
#endif
