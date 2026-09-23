#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Sight.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxAIController.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"

namespace
{
/** 同步自动化测试里模拟独立引擎帧，使世界计时器逐帧执行并避免大 Delta 被钳制。 */
void AdvanceIntegrationWorld(UWorld& World, float Seconds)
{
	while (Seconds > UE_KINDA_SMALL_NUMBER)
	{
		const float Step = FMath::Min(Seconds, 0.05f);
		++GFrameCounter;
		World.Tick(LEVELTICK_All, Step);
		Seconds -= Step;
	}
}

/** 创建不依赖项目地图和用户资产的四入口最小分析图。 */
ULxAIBehaviorTreeAsset* MakeIntegrationAsset()
{
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	const ELxAIBehaviorState States[] = {ELxAIBehaviorState::Idle, ELxAIBehaviorState::Alert,
		ELxAIBehaviorState::Combat, ELxAIBehaviorState::Combat};
	const ELxAIBehaviorAction Actions[] = {ELxAIBehaviorAction::Wait, ELxAIBehaviorAction::Alert,
		ELxAIBehaviorAction::Defend, ELxAIBehaviorAction::Defend};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* Entry = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* State = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* Action = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		for (ULxAIBehaviorTreeNodeData* Node : {Entry, State, Phase, Action})
		{
			Node->NodeId = FGuid::NewGuid();
			Asset->Nodes.Add(Node);
		}
		Entry->Kind = ELxAIBehaviorNodeKind::Entry;
		Entry->Entry = static_cast<ELxAIBehaviorEntry>(Index);
		Entry->EntryPriority = Asset->GetDefaultEntryPriority(Entry->Entry);
		Entry->Children.Add(State->NodeId);
		State->Kind = ELxAIBehaviorNodeKind::State;
		State->State = States[Index];
		State->Children.Add(Phase->NodeId);
		Phase->Kind = ELxAIBehaviorNodeKind::Phase;
		Phase->State = States[Index];
		Phase->Children.Add(Action->NodeId);
		Action->Kind = ELxAIBehaviorNodeKind::Action;
		Action->Action = Actions[Index];
		Asset->Roots.Add(Entry->NodeId);
	}
	return Asset;
}
}

/** 通过真实世界的控制器占有和公开感知入口验证新旧模式隔离与会话生命周期。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIAnalysisControllerIntegrationTest,
	"LxARPG.AIControlConfig.ControllerIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIAnalysisControllerIntegrationTest::RunTest(const FString& Parameters)
{
	ULxAIBehaviorTreeAsset* Asset = MakeIntegrationAsset();
	Asset->Perception.bEnableSight = false;
	Asset->Perception.bEnableHearing = false;
	FText Error;
	if (!TestTrue(TEXT("独立四入口测试资产有效"), Asset->ValidateConfiguration(Error)))
	{
		AddError(Error.ToString());
		return false;
	}
	FObjectPropertyBase* AssetProperty = FindFProperty<FObjectPropertyBase>(ALxAICharacter::StaticClass(), TEXT("AIBehaviorTreeAsset"));
	if (!TestNotNull(TEXT("角色类型存在类默认值配置"), AssetProperty)) return false;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->EndPlay(EEndPlayReason::Quit);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	// 独立 CreateWorld 不会自动创建 AI 系统；先建系统，再注册角色和感知组件。
	if (!TestNotNull(TEXT("临时世界创建真实AI系统"), World->CreateAISystem())) return false;
	if (!TestNotNull(TEXT("临时世界包含感知系统"), UAIPerceptionSystem::GetCurrent(World))) return false;
	World->InitializeActorsForPlay(FURL());
	ALxAICharacter* FirstPawn = World->SpawnActor<ALxAICharacter>();
	ALxAICharacter* SecondPawn = World->SpawnActor<ALxAICharacter>();
	ALxAICharacter* Enemy = World->SpawnActor<ALxAICharacter>();
	ALxAIController* First = World->SpawnActor<ALxAIController>();
	ALxAIController* Second = World->SpawnActor<ALxAIController>();
	if (!TestNotNull(TEXT("第一角色"), FirstPawn) || !TestNotNull(TEXT("第二角色"), SecondPawn) ||
		!TestNotNull(TEXT("目标角色"), Enemy) || !TestNotNull(TEXT("第一控制器"), First) ||
		!TestNotNull(TEXT("第二控制器"), Second)) return false;
	// 原生构造器会重置 CDO 临时赋值；受控测试通过反射在占有前安装资产。
	AssetProperty->SetObjectPropertyValue_InContainer(FirstPawn, Asset);
	AssetProperty->SetObjectPropertyValue_InContainer(SecondPawn, Asset);
	if (!TestEqual(TEXT("第一角色确实持有测试资产"), FirstPawn->GetAIBehaviorTreeAsset(), Asset) ||
		!TestEqual(TEXT("第二角色确实持有测试资产"), SecondPawn->GetAIBehaviorTreeAsset(), Asset) ||
		!TestTrue(TEXT("第一角色自动控制总开关开启"), FirstPawn->GetAIControlConfig().bEnableAutomaticControl) ||
		!TestTrue(TEXT("第二角色自动控制总开关开启"), SecondPawn->GetAIControlConfig().bEnableAutomaticControl)) return false;
	First->Possess(FirstPawn);
	Second->Possess(SecondPawn);
	if (!TestTrue(TEXT("第一控制器已初始化有效分析分支"), First->GetCurrentAnalysisDecision().bHasBranch) ||
		!TestTrue(TEXT("第二控制器已初始化有效分析分支"), Second->GetCurrentAnalysisDecision().bHasBranch)) return false;
	auto ReportDecisionState = [this, World](const TCHAR* Stage, const ALxAIController* Controller)
	{
		const FLxAIAnalysisDecision Decision = Controller->GetCurrentAnalysisDecision();
		AddInfo(FString::Printf(TEXT("%s: Entry=%d Branch=%d Memory=%d WorldTime=%.3f TimerActive=%d TimerPending=%d TimerTickedThisFrame=%d Frame=%llu"),
			Stage, static_cast<int32>(Decision.Entry), Decision.bHasBranch ? 1 : 0,
			Controller->GetTargetMemoryCount(), World->GetTimeSeconds(),
			World->GetTimerManager().IsTimerActive(Controller->AutomaticDecisionTimer) ? 1 : 0,
			World->GetTimerManager().IsTimerPending(Controller->AutomaticDecisionTimer) ? 1 : 0,
			World->GetTimerManager().HasBeenTickedThisFrame() ? 1 : 0,
			static_cast<unsigned long long>(GFrameCounter)));
	};
	ReportDecisionState(TEXT("初次占有"), First);
	if (!TestTrue(TEXT("第一控制器在世界计时器注册分析更新"),
		World->GetTimerManager().TimerExists(First->AutomaticDecisionTimer)) ||
		!TestTrue(TEXT("第二控制器在世界计时器注册分析更新"),
			World->GetTimerManager().TimerExists(Second->AutomaticDecisionTimer))) return false;
	TestEqual(TEXT("第一控制器初始平静入口"), First->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::Calm);
	TestEqual(TEXT("第二控制器独立平静入口"), Second->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::Calm);
	TestEqual(TEXT("新模式不启动旧行为"), First->GetCurrentAction(), ELxAIActionType::None);
	Enemy->SetActorLocation(FirstPawn->GetActorLocation() + FVector(2000, 0, 0));
	First->ReportPerceivedTarget(Enemy, ELxAIPerceptionSource::Damage, true);
	Enemy->SetActorLocation(FirstPawn->GetActorLocation() + FVector(100, 0, 0));
	const double BeforeFirstTick = World->GetTimeSeconds();
	AdvanceIntegrationWorld(*World, 0.21f);
	TestTrue(TEXT("临时世界真实时间向前推进"), World->GetTimeSeconds() > BeforeFirstTick);
	ReportDecisionState(TEXT("伤害来源记忆后"), First);
	TestEqual(TEXT("记忆使用最后已知距离，不偷读移动后的位置"), First->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::EnemyFound);
	TestEqual(TEXT("第二控制器未共享目标"), Second->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::Calm);
	UFunction* DamageFunction = FirstPawn->FindFunction(TEXT("HandleAIReceivedDamage"));
	if (!TestNotNull(TEXT("角色效果受击入口可反射调用"), DamageFunction)) return false;
	struct FDamageEventParams { FLxDamageReceiveResult Result; AActor* AttackerActor = nullptr; } DamageEvent;
	FirstPawn->ProcessEvent(DamageFunction, &DamageEvent);
	ReportDecisionState(TEXT("零承伤后"), First);
	TestEqual(TEXT("零承伤不触发受击"), First->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::EnemyFound);
	DamageEvent.Result.HealthDamageValue = 1.0f;
	FirstPawn->ProcessEvent(DamageFunction, &DamageEvent);
	ReportDecisionState(TEXT("有效无来源承伤后"), First);
	TestEqual(TEXT("有效且无来源的承伤进入受击入口"), First->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::Attacked);
	First->CompleteAttackedResponse();
	TestFalse(TEXT("响应完成后不继续保持受击入口"), First->GetCurrentAnalysisDecision().Entry == ELxAIBehaviorEntry::Attacked);
	AdvanceIntegrationWorld(*World, 5.1f);
	ReportDecisionState(TEXT("警觉与记忆到期后"), First);
	TestEqual(TEXT("真实时间推进后清理受击来源记忆"), First->GetTargetMemoryCount(), 0);
	TestEqual(TEXT("警觉和记忆到期恢复平静"), First->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::Calm);
	First->UnPossess();
	TestFalse(TEXT("反占有清除会话决策"), First->GetCurrentAnalysisDecision().bHasBranch);
	TestEqual(TEXT("反占有清除私有记忆"), First->GetTargetMemoryCount(), 0);
	ULxAIBehaviorTreeNodeData* FoundEntry = nullptr;
	for (ULxAIBehaviorTreeNodeData* Node : Asset->Nodes)
	{
		if (Node->Kind == ELxAIBehaviorNodeKind::Entry && Node->Entry == ELxAIBehaviorEntry::EnemyFound)
		{
			FoundEntry = Node;
			break;
		}
	}
	if (!TestNotNull(TEXT("发现敌人入口"), FoundEntry)) return false;
	FoundEntry->EntryPriority = 400;
	ALxAICharacter* NewAttacker = World->SpawnActor<ALxAICharacter>();
	if (!TestNotNull(TEXT("新攻击者"), NewAttacker)) return false;
	NewAttacker->SetActorLocation(SecondPawn->GetActorLocation() + FVector(2000, 0, 0));
	DamageEvent.AttackerActor = NewAttacker;
	SecondPawn->ProcessEvent(DamageFunction, &DamageEvent);
	ReportDecisionState(TEXT("高优先级新攻击者受击后"), Second);
	TestEqual(TEXT("同次有效受击先记录新攻击者，立即按高优先级发现入口决策"),
		Second->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::EnemyFound);
	FoundEntry->EntryPriority = 100;

	// 以下通过公开报告入口验证记忆时效；不宣称通过了真实视线或听觉刺激。
	Asset->Perception.bEnableSight = true;
	Asset->Perception.bEnableHearing = true;
	Asset->Perception.bDetectNeutrals = true;
	Asset->Perception.SightRadiusMeters = 1.0f;
	Asset->Perception.LoseSightRadiusMeters = 1.5f;
	Asset->Perception.HearingRadiusMeters = 1.0f;
	Asset->Perception.SightHalfAngleDegrees = 45.0f;
	Asset->Perception.SightMemorySeconds = 0.2f;
	Asset->Perception.HearingMemorySeconds = 0.5f;
	Enemy->SetActorLocation(SecondPawn->GetActorLocation() + FVector(2000, 0, 0));
	Second->UnPossess();
	Second->Possess(SecondPawn);
	UAIPerceptionComponent* Perception = Second->GetAIPerceptionComponent();
	if (!TestNotNull(TEXT("控制器真实感知组件"), Perception)) return false;
	const FAISenseID SightSenseId = UAISense::GetSenseID<UAISense_Sight>();
	if (!TestTrue(TEXT("视觉Sense已经注册"), SightSenseId.IsValid()) ||
		!TestTrue(TEXT("真实感知组件已经注册"), Perception->IsRegistered()) ||
		!TestNotNull(TEXT("真实感知组件已配置视觉Sense"), Perception->GetSenseConfig(SightSenseId))) return false;
	const UAISense_Sight* SightSense = GetDefault<UAISense_Sight>();
	const FAIStimulus SightStimulus(*SightSense, 1.0f, Enemy->GetActorLocation(),
		SecondPawn->GetActorLocation());
	Perception->RegisterStimulus(Enemy, SightStimulus);
	Perception->ProcessStimuli();
	TArray<AActor*> EnginePerceivedActors;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), EnginePerceivedActors);
	TestTrue(TEXT("引擎感知缓存含旧Pawn期间处理的视觉目标"), EnginePerceivedActors.Contains(Enemy));
	Second->UnPossess();
	EnginePerceivedActors.Reset();
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), EnginePerceivedActors);
	TestFalse(TEXT("反占有清除引擎感知缓存"), EnginePerceivedActors.Contains(Enemy));
	Second->Possess(SecondPawn);
	Perception->RegisterStimulus(Enemy, SightStimulus);
	Second->UnPossess();
	Second->Possess(SecondPawn);
	Perception->ProcessStimuli();
	EnginePerceivedActors.Reset();
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), EnginePerceivedActors);
	TestFalse(TEXT("换Pawn前排队的刺激不在新占有期重建旧缓存"), EnginePerceivedActors.Contains(Enemy));
	Second->ReportPerceivedTarget(Enemy, ELxAIPerceptionSource::Sight, true);
	Second->ReportPerceivedTarget(Enemy, ELxAIPerceptionSource::Hearing);
	AdvanceIntegrationWorld(*World, 0.3f);
	ReportDecisionState(TEXT("视觉记忆到期后"), Second);
	TestEqual(TEXT("视觉记忆过期而听觉记忆仍有效"), Second->GetTargetMemoryCount(), 1);
	TestEqual(TEXT("有效听觉记忆保留已知敌人"), Second->GetCurrentAnalysisDecision().Entry, ELxAIBehaviorEntry::EnemyFound);
	AdvanceIntegrationWorld(*World, 0.3f);
	ReportDecisionState(TEXT("听觉记忆到期后"), Second);
	TestEqual(TEXT("两种来源分别过期后清理目标"), Second->GetTargetMemoryCount(), 0);
	Asset->Perception.HearingMemorySeconds = 0.0f;
	Second->UnPossess();
	Second->Possess(SecondPawn);
	Second->ReportPerceivedTarget(Enemy, ELxAIPerceptionSource::Hearing, true);
	AdvanceIntegrationWorld(*World, 1.0f);
	TestEqual(TEXT("零秒听觉记忆不因超时遗忘"), Second->GetTargetMemoryCount(), 1);

	FObjectPropertyBase* SightProperty = FindFProperty<FObjectPropertyBase>(ALxAIController::StaticClass(), TEXT("SightConfig"));
	if (!TestNotNull(TEXT("视觉配置反射属性"), SightProperty)) return false;
	UAISenseConfig_Sight* Sight = Cast<UAISenseConfig_Sight>(SightProperty->GetObjectPropertyValue_InContainer(Second));
	if (!TestNotNull(TEXT("视觉配置对象"), Sight)) return false;
	TestEqual(TEXT("新资产半角已应用"), Sight->PeripheralVisionAngleDegrees, 45.0f);
	Second->UnPossess();
	ALxAICharacter* LegacyPawn = World->SpawnActor<ALxAICharacter>();
	if (!TestNotNull(TEXT("旧模式角色"), LegacyPawn)) return false;
	AssetProperty->SetObjectPropertyValue_InContainer(LegacyPawn, nullptr);
	if (!TestNull(TEXT("旧模式角色没有分析资产"), LegacyPawn->GetAIBehaviorTreeAsset())) return false;
	Second->Possess(LegacyPawn);
	TestEqual(TEXT("重新占有旧角色后恢复默认视觉半角"), Sight->PeripheralVisionAngleDegrees, 90.0f);
	TestEqual(TEXT("重新占有旧角色后恢复旧视觉距离"), Sight->SightRadius,
		LegacyPawn->GetAIControlConfig().SightRadius * 100.0f);
	return true;
}

#endif
