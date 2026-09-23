#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/AI/LxAIControlAnalysis.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include <limits>

namespace
{
/** 创建最小可执行参数有效的配置节点，不依赖用户资产或技能标签。 */
ULxAIBehaviorTreeNodeData* AddAnalysisTestNode(ULxAIBehaviorTreeAsset& Asset, ELxAIBehaviorNodeKind Kind, ELxAIBehaviorState State)
{
	ULxAIBehaviorTreeNodeData* Node = NewObject<ULxAIBehaviorTreeNodeData>(&Asset);
	Node->NodeId = FGuid::NewGuid();
	Node->Kind = Kind;
	Node->State = State;
	Asset.Nodes.Add(Node);
	return Node;
}

/** 为分析测试创建平静闲置、发现警戒、靠近战斗、受击战斗或逃跑四条规则。 */
ULxAIBehaviorTreeAsset* MakeAnalysisTestConfig()
{
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	TArray<ULxAIBehaviorTreeNodeData*> Entries;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* Entry = AddAnalysisTestNode(*Asset, ELxAIBehaviorNodeKind::Entry, ELxAIBehaviorState::Idle);
		Entry->Entry = static_cast<ELxAIBehaviorEntry>(Index);
		Entry->EntryPriority = Asset->GetDefaultEntryPriority(Entry->Entry);
		Entries.Add(Entry);
		Asset->Roots.Add(Entry->NodeId);
	}
	const ELxAIBehaviorState States[] = {ELxAIBehaviorState::Idle, ELxAIBehaviorState::Alert, ELxAIBehaviorState::Combat, ELxAIBehaviorState::Flee};
	const ELxAIBehaviorAction Actions[] = {ELxAIBehaviorAction::Wait, ELxAIBehaviorAction::Alert, ELxAIBehaviorAction::Defend, ELxAIBehaviorAction::RandomFlee};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* State = AddAnalysisTestNode(*Asset, ELxAIBehaviorNodeKind::State, States[Index]);
		ULxAIBehaviorTreeNodeData* Phase = AddAnalysisTestNode(*Asset, ELxAIBehaviorNodeKind::Phase, States[Index]);
		ULxAIBehaviorTreeNodeData* Action = AddAnalysisTestNode(*Asset, ELxAIBehaviorNodeKind::Action, States[Index]);
		State->Order = Index;
		State->Children.Add(Phase->NodeId);
		Phase->Children.Add(Action->NodeId);
		Action->Action = Actions[Index];
		if (States[Index] == ELxAIBehaviorState::Combat) State->HealthRange.Min = Phase->HealthRange.Min = 0.3f;
		if (States[Index] == ELxAIBehaviorState::Flee) State->HealthRange.Max = Phase->HealthRange.Max = 0.29f;
		for (ELxAIBehaviorEntry Type : Asset->GetDefaultEntriesForState(States[Index]))
			Entries[static_cast<int32>(Type)]->Children.Add(State->NodeId);
	}
	return Asset;
}

/** 按入口分类查找测试配置对象。 */
ULxAIBehaviorTreeNodeData* FindAnalysisTestEntry(ULxAIBehaviorTreeAsset& Asset, ELxAIBehaviorEntry Entry)
{
	for (ULxAIBehaviorTreeNodeData* Node : Asset.Nodes)
		if (Node->Kind == ELxAIBehaviorNodeKind::Entry && Node->Entry == Entry) return Node;
	return nullptr;
}
}

/** 用真实独立会话验证入口选择、事件生命周期和共享资产隔离。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIControlAnalysisRoutingTest, "LxARPG.AIControlConfig.AnalysisRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIControlAnalysisRoutingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	World->InitializeActorsForPlay(FURL());
	AActor* Enemy = World->SpawnActor<AActor>();
	AActor* OtherEnemy = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("真实世界目标"), Enemy) || !TestNotNull(TEXT("第二个目标"), OtherEnemy)) return false;
	ULxAIBehaviorTreeAsset* Config = MakeAnalysisTestConfig();
	FText Error;
	if (!TestTrue(TEXT("四入口共享战斗状态的测试资产有效"), Config->ValidateConfiguration(Error)))
	{
		AddError(Error.ToString());
		return false;
	}
	ULxAIControlAnalysis* First = NewObject<ULxAIControlAnalysis>();
	ULxAIControlAnalysis* Second = NewObject<ULxAIControlAnalysis>();
	First->Initialize(Config);
	Second->Initialize(Config);
	FLxAIAnalysisDecision Decision = First->Evaluate(nullptr, 0, 1.0f, 0);
	TestTrue(TEXT("无敌无受击时选择平静分支"), Decision.bHasBranch && Decision.Entry == ELxAIBehaviorEntry::Calm);
	TestTrue(TEXT("首次分支需要启动"), Decision.bStartNewBehavior);
	TestFalse(TEXT("稳定平静不会每次分析重启"), First->Evaluate(nullptr, 0, 1, 0.1f).bStartNewBehavior);
	Decision = First->Evaluate(Enemy, 20, 1, 0);
	TestEqual(TEXT("远处已知敌人选择发现入口"), Decision.Entry, ELxAIBehaviorEntry::EnemyFound);
	Decision = First->Evaluate(Enemy, 5, 1, 0);
	TestEqual(TEXT("进入距离边界触发靠近"), Decision.Entry, ELxAIBehaviorEntry::EnemyNear);
	const FGuid SharedCombatState = Decision.StateId;
	TestEqual(TEXT("滞回区间保持靠近"), First->Evaluate(Enemy, 6, 1, 0).Entry, ELxAIBehaviorEntry::EnemyNear);
	TestEqual(TEXT("退出距离边界保持靠近"), First->Evaluate(Enemy, 7, 1, 0).Entry, ELxAIBehaviorEntry::EnemyNear);
	TestEqual(TEXT("超过退出距离切回发现"), First->Evaluate(Enemy, 7.01f, 1, 0).Entry, ELxAIBehaviorEntry::EnemyFound);
	TestEqual(TEXT("从外侧进入滞回区间不会提前靠近"), First->Evaluate(Enemy, 6, 1, 0).Entry, ELxAIBehaviorEntry::EnemyFound);
	First->Evaluate(Enemy, 4, 1, 0);
	TestEqual(TEXT("更换目标会重置距离滞回"), First->Evaluate(OtherEnemy, 6, 1, 0).Entry, ELxAIBehaviorEntry::EnemyFound);
	Decision = First->Evaluate(Enemy, 4, 0.2f, 0);
	TestEqual(TEXT("靠近分支血量不符则回退发现入口"), Decision.Entry, ELxAIBehaviorEntry::EnemyFound);
	First->NotifyAttacked();
	Decision = First->Evaluate(Enemy, 4, 0.2f, 0);
	TestEqual(TEXT("受击优先于靠近和发现"), Decision.Entry, ELxAIBehaviorEntry::Attacked);
	TestEqual(TEXT("低血量受击选择逃跑状态"), Config->FindNode(Decision.StateId)->State, ELxAIBehaviorState::Flee);
	const FGuid ActivePhase = Decision.PhaseId;
	First->NotifyAttacked();
	Decision = First->Evaluate(Enemy, 4, 0.2f, 0.1f);
	TestEqual(TEXT("连续受击保持当前阶段"), Decision.PhaseId, ActivePhase);
	TestFalse(TEXT("连续受击不重启同一响应"), Decision.bStartNewBehavior);
	TestEqual(TEXT("另一会话不共享受击事件"), Second->Evaluate(nullptr, 0, 1, 0).Entry, ELxAIBehaviorEntry::Calm);
	TestFalse(TEXT("另一会话不共享警觉时间"), Second->IsAlertAfterAttack());
	First->CompleteAttackedResponse();
	Decision = First->Evaluate(nullptr, 0, 0.2f, 0);
	TestFalse(TEXT("无已知敌人但仍警觉时不会进入平静"), Decision.bHasBranch);
	Decision = First->Evaluate(nullptr, 0, 0.2f, 5);
	TestEqual(TEXT("警觉到期后允许恢复平静"), Decision.Entry, ELxAIBehaviorEntry::Calm);
	TestTrue(TEXT("警觉到期确实存在分支"), Decision.bHasBranch);
	First->NotifyAttacked();
	Decision = First->Evaluate(Enemy, 3, 1, 0);
	TestEqual(TEXT("新的受击可以再次产生响应"), Decision.Entry, ELxAIBehaviorEntry::Attacked);
	TestEqual(TEXT("受击与靠近复用同一个战斗状态"), Decision.StateId, SharedCombatState);
	First->NotifyAttacked();
	First->Evaluate(Enemy, 3, 1, 3);
	TestTrue(TEXT("受击刷新警觉时间"), First->IsAlertAfterAttack());
	First->CompleteAttackedResponse();
	Decision = First->Evaluate(Enemy, 3, 1, 3);
	TestEqual(TEXT("完成后连续受击不残留待重放事件"), Decision.Entry, ELxAIBehaviorEntry::EnemyNear);
	TestFalse(TEXT("警觉时间独立于响应生命周期到期"), First->IsAlertAfterAttack());
	First->Initialize(Config);
	First->Evaluate(Enemy, 20, 1, 0);
	First->NotifyAttacked();
	Decision = First->Evaluate(Enemy, 3, 1, 0, false);
	TestEqual(TEXT("不可打断时保持当前发现分支"), Decision.Entry, ELxAIBehaviorEntry::EnemyFound);
	TestFalse(TEXT("不可打断时没有新行为启动"), Decision.bStartNewBehavior);
	TestEqual(TEXT("解除锁后丢弃的受击脉冲不会重放"), First->Evaluate(Enemy, 3, 1, 0).Entry, ELxAIBehaviorEntry::EnemyNear);
	ULxAIBehaviorTreeNodeData* Found = FindAnalysisTestEntry(*Config, ELxAIBehaviorEntry::EnemyFound);
	Found->EntryPriority = 400;
	First->NotifyAttacked();
	TestEqual(TEXT("调整入口优先级实际改变选择"), First->Evaluate(Enemy, 3, 1, 0).Entry, ELxAIBehaviorEntry::EnemyFound);
	Found->EntryPriority = 100;
	First->NotifyAttacked();
	Decision = First->Evaluate(Enemy, 3, 0.295f, 0);
	TestEqual(TEXT("受击和靠近均无符合血量分支时继续回退发现"), Decision.Entry, ELxAIBehaviorEntry::EnemyFound);
	TestFalse(TEXT("无可用受击分支时不挂起一次响应"), First->IsAttackedResponseActive());
	Enemy->Destroy();
	First->Initialize(Config);
	TestEqual(TEXT("已销毁目标不能继续维持发现入口"), First->Evaluate(Enemy, 3, 1, 0).Entry, ELxAIBehaviorEntry::Calm);
	TestFalse(TEXT("非数值生命比例不产生有效决策"), First->Evaluate(nullptr, 0, std::numeric_limits<float>::quiet_NaN(), 0).bHasBranch);
	TestFalse(TEXT("负时间步不推进分析"), First->Evaluate(nullptr, 0, 1, -1).bHasBranch);
	TestFalse(TEXT("负敌人距离不产生有效决策"), First->Evaluate(OtherEnemy, -1, 1, 0).bHasBranch);
	Config->Analysis.NearExitDistanceMeters = Config->Analysis.NearEnterDistanceMeters;
	TestFalse(TEXT("拒绝没有间隔的距离滞回配置"), Config->Analysis.ValidateConfiguration(Error));
	TestFalse(TEXT("无效分析配置不产生分支"), Second->Evaluate(nullptr, 0, 1, 0).bHasBranch);
	Config->Analysis = FLxAIAnalysisConfig();
	TestTrue(TEXT("分析不改写共享入口或节点数据"), Config->ValidateConfiguration(Error));
	return true;
}

#endif
