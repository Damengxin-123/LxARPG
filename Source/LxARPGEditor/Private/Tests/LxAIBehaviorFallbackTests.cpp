#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorTreeExecutor.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

/** 验证无法执行的行为在同轮跳过，正常运行的行为保留，全部失败时有限结束。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIBehaviorFallbackTest,
	"LxARPG.AIControlConfig.BehaviorFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIBehaviorFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	ALxAICharacter* Character = World->SpawnActor<ALxAICharacter>();
	if (!TestNotNull(TEXT("创建测试角色"), Character)) return false;
	ULxAIBehaviorTreeExecutor* Executor = NewObject<ULxAIBehaviorTreeExecutor>(Character);
	Executor->Initialize(Character);
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	Phase->Kind = ELxAIBehaviorNodeKind::Phase;
	Phase->NodeId = FGuid::NewGuid();
	Asset->Nodes.Add(Phase);
	const ELxAIBehaviorAction Actions[] = {
		ELxAIBehaviorAction::MeleeSkill, ELxAIBehaviorAction::Alert,
		ELxAIBehaviorAction::Defend, ELxAIBehaviorAction::PointPatrol,
		ELxAIBehaviorAction::RouteFlee, ELxAIBehaviorAction::RandomFlee,
		ELxAIBehaviorAction::Wait};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
	{
		ULxAIBehaviorTreeNodeData* Node = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		Node->Kind = ELxAIBehaviorNodeKind::Action;
		Node->NodeId = FGuid::NewGuid();
		Node->Action = Actions[Index];
		Node->Order = Index;
		Node->WaitSeconds = 0.0f;
		Asset->Nodes.Add(Node);
		Phase->Children.Add(Node->NodeId);
	}
	FLxAIAnalysisDecision Decision;
	Decision.bHasBranch = true;
	Decision.PhaseId = Phase->NodeId;
	Decision.Entry = ELxAIBehaviorEntry::Attacked;
	// 无敌人、场景点和导航网格，前六个行为均无法执行，末尾待机应在同轮开始。
	TestFalse(TEXT("后续待机保持阶段运行"), Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f));
	TestEqual(TEXT("同轮连续跳过失败行为，到达最后一个待机节点"), Executor->GetCurrentActionNodeId(), Phase->Children.Last());
	Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestEqual(TEXT("正常待机不会被当作无法执行而跳过"), Executor->GetCurrentActionNodeId(), Phase->Children.Last());

	Phase->Children.Pop();
	Executor->ResetExecution();
	TestTrue(TEXT("全部行为失败时本轮结束受击响应"), Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f));
	TestFalse(TEXT("全部失败后清空当前行为"), Executor->GetCurrentActionNodeId().IsValid());
	return true;
}

/** 独立有效资产验证安全距离会真正退出分析分支，且不会因持续低血量重入。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIRandomFleeSafetyTest,
	"LxARPG.AIControlConfig.RandomFleeSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIRandomFleeSafetyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AActor* Enemy = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("创建已知敌人"), Enemy)) return false;
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	ULxAIBehaviorTreeNodeData* State = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	ULxAIBehaviorTreeNodeData* Action = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	for (ULxAIBehaviorTreeNodeData* Node : {State, Phase, Action})
	{
		Node->NodeId = FGuid::NewGuid();
		Node->State = ELxAIBehaviorState::Flee;
		Asset->Nodes.Add(Node);
	}
	State->Kind = ELxAIBehaviorNodeKind::State;
	Phase->Kind = ELxAIBehaviorNodeKind::Phase;
	Action->Kind = ELxAIBehaviorNodeKind::Action;
	Action->Action = ELxAIBehaviorAction::RandomFlee;
	Action->FleeSafeDistanceMeters = 15.0f;
	State->Children.Add(Phase->NodeId);
	Phase->Children.Add(Action->NodeId);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* Entry = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		Entry->NodeId = FGuid::NewGuid();
		Entry->Kind = ELxAIBehaviorNodeKind::Entry;
		Entry->Entry = static_cast<ELxAIBehaviorEntry>(Index);
		Entry->EntryPriority = Asset->GetDefaultEntryPriority(Entry->Entry);
		if (Entry->Entry == ELxAIBehaviorEntry::Attacked || Entry->Entry == ELxAIBehaviorEntry::EnemyFound)
			Entry->Children.Add(State->NodeId);
		Asset->Nodes.Add(Entry);
		Asset->Roots.Add(Entry->NodeId);
	}
	FText Error;
	if (!TestTrue(TEXT("安全距离测试资产有效"), Asset->ValidateConfiguration(Error))) { AddError(Error.ToString()); return false; }
	ULxAIControlAnalysis* Session = NewObject<ULxAIControlAnalysis>();
	Session->Initialize(Asset);
	Session->NotifyAttacked();
	TestTrue(TEXT("安全距离以内进入逃跑"), Session->Evaluate(Enemy, 14.99f, 0.2f, 0).bHasBranch);
	TestFalse(TEXT("达到配置的安全距离退出逃跑"), Session->Evaluate(Enemy, 15.0f, 0.2f, 0).bHasBranch);
	TestFalse(TEXT("低血量与发现敌人入口不会导致立即重入"), Session->Evaluate(Enemy, 16.0f, 0.2f, 0).bHasBranch);
	TestTrue(TEXT("敌人重新靠近后可再次选择逃跑"), Session->Evaluate(Enemy, 5.0f, 0.2f, 0).bHasBranch);
	TestFalse(TEXT("失去全部已知敌人后退出逃跑"), Session->Evaluate(nullptr, 0.0f, 0.2f, 0).bHasBranch);
	Action->FleeSafeDistanceMeters = 0.0f;
	TestFalse(TEXT("安全距离不能为零"), Action->ValidateConfiguration(Error));
	return true;
}

/** 使用真实分析与执行器验证执行锁抵御受击，并在失败、完成和重置后释放。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIBehaviorInterruptTest,
	"LxARPG.AIControlConfig.BehaviorInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIBehaviorInterruptTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	ALxAICharacter* Character = World->SpawnActor<ALxAICharacter>();
	AActor* Enemy = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("测试角色"), Character) || !TestNotNull(TEXT("测试敌人"), Enemy)) return false;
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	ULxAIBehaviorTreeNodeData* LockedAction = nullptr;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* Entry = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		Entry->Kind = ELxAIBehaviorNodeKind::Entry;
		Entry->Entry = static_cast<ELxAIBehaviorEntry>(Index);
		Entry->NodeId = FGuid::NewGuid();
		Entry->EntryPriority = Asset->GetDefaultEntryPriority(Entry->Entry);
		Asset->Roots.Add(Entry->NodeId);
		Asset->Nodes.Add(Entry);
		if (Entry->Entry != ELxAIBehaviorEntry::EnemyFound && Entry->Entry != ELxAIBehaviorEntry::Attacked) continue;
		ULxAIBehaviorTreeNodeData* State = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* Action = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		for (ULxAIBehaviorTreeNodeData* Node : {State, Phase, Action})
		{
			Node->NodeId = FGuid::NewGuid();
			Node->State = Entry->Entry == ELxAIBehaviorEntry::EnemyFound ? ELxAIBehaviorState::Alert : ELxAIBehaviorState::Combat;
			Asset->Nodes.Add(Node);
		}
		State->Kind = ELxAIBehaviorNodeKind::State;
		Phase->Kind = ELxAIBehaviorNodeKind::Phase;
		Action->Kind = ELxAIBehaviorNodeKind::Action;
		Action->Action = Entry->Entry == ELxAIBehaviorEntry::EnemyFound ? ELxAIBehaviorAction::Alert : ELxAIBehaviorAction::Defend;
		Entry->Children.Add(State->NodeId);
		State->Children.Add(Phase->NodeId);
		Phase->Children.Add(Action->NodeId);
		if (Entry->Entry == ELxAIBehaviorEntry::EnemyFound) LockedAction = Action;
	}
	FText Error;
	if (!TestTrue(TEXT("执行锁测试配置有效"), Asset->ValidateConfiguration(Error))) { AddError(Error.ToString()); return false; }
	TestTrue(TEXT("新行为默认可以打断"), LockedAction->bCanInterrupt);
	LockedAction->bCanInterrupt = false;
	ULxAIBehaviorTreeExecutor* Executor = NewObject<ULxAIBehaviorTreeExecutor>(Character);
	ULxAIControlAnalysis* Session = NewObject<ULxAIControlAnalysis>();
	Executor->Initialize(Character);
	Session->Initialize(Asset);
	TestTrue(TEXT("尚未执行时不会锁定分析"), Executor->CanInterruptCurrentAction());
	const FVector EnemyLocation = Character->GetActorLocation() + FVector(400, 0, 0);
	FLxAIAnalysisDecision Decision = Session->Evaluate(Enemy, 4.0f, 1.0f, 0);
	Executor->TickExecution(Asset, Decision, Enemy, EnemyLocation, 0.1f);
	TestFalse(TEXT("不可打断行为运行后持有锁"), Executor->CanInterruptCurrentAction());
	Session->NotifyAttacked();
	Decision = Session->Evaluate(Enemy, 4.0f, 1.0f, 0.1f, Executor->CanInterruptCurrentAction());
	TestEqual(TEXT("受击不会抢占正在运行的行为"), Decision.Entry, ELxAIBehaviorEntry::EnemyFound);
	TestFalse(TEXT("不会重启当前阶段"), Decision.bStartNewBehavior);
	Executor->TickExecution(Asset, Decision, Enemy, EnemyLocation, 0.1f);
	TestEqual(TEXT("执行器持续保持原行为"), Executor->GetCurrentActionNodeId(), LockedAction->NodeId);
	Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestTrue(TEXT("明确失败后立即释放锁"), Executor->CanInterruptCurrentAction());
	Session->NotifyAttacked();
	TestEqual(TEXT("释放后新受击可抢占"), Session->Evaluate(Enemy, 4.0f, 1.0f, 0, Executor->CanInterruptCurrentAction()).Entry, ELxAIBehaviorEntry::Attacked);

	// 直接使用执行器验证正常完成也会释放锁，分析状态不会影响此生命周期检查。
	Executor->ResetExecution();
	Session->Initialize(Asset);
	Decision = Session->Evaluate(Enemy, 4.0f, 1.0f, 0);
	LockedAction->Action = ELxAIBehaviorAction::RandomFlee;
	LockedAction->FleeSafeDistanceMeters = 3.0f;
	Executor->TickExecution(Asset, Decision, Enemy, EnemyLocation, 0.1f);
	TestTrue(TEXT("达到安全距离正常完成后释放锁"), Executor->CanInterruptCurrentAction());
	LockedAction->Action = ELxAIBehaviorAction::Alert;
	Decision.bStartNewBehavior = false;
	Executor->TickExecution(Asset, Decision, Enemy, EnemyLocation, 0.1f);
	TestFalse(TEXT("再次运行可重新锁定"), Executor->CanInterruptCurrentAction());
	const FLxAIAnalysisDecision DeathDecision = Session->Evaluate(Enemy, 4.0f, 0.0f, 0, Executor->CanInterruptCurrentAction());
	TestEqual(TEXT("死亡事件仍可抢占不可打断行为"), DeathDecision.Entry, ELxAIBehaviorEntry::CharacterDeath);
	Executor->ResetExecution();
	TestTrue(TEXT("重置释放执行锁"), Executor->CanInterruptCurrentAction());
	return true;
}

#endif
