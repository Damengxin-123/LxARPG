#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "LxARPG/LxSource/Model/AI/LxAIControlAnalysis.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include "LxARPG/LxSource/World/AINavigation/LxAINavigationTags.h"

/** 验证追击起点、边界退出、返回保护和再次追击的独立生命周期。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIChaseDistanceTest, "LxARPG.AIControlConfig.ChaseDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIChaseDistanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AActor* Enemy = World->SpawnActor<AActor>();
	AActor* OtherEnemy = World->SpawnActor<AActor>();
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	TArray<ULxAIBehaviorTreeNodeData*> States;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		ULxAIBehaviorTreeNodeData* State = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		ULxAIBehaviorTreeNodeData* Action = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		for (ULxAIBehaviorTreeNodeData* Node : {State, Phase, Action})
		{
			Node->NodeId = FGuid::NewGuid();
			Node->State = Index == 0 ? ELxAIBehaviorState::Patrol : ELxAIBehaviorState::Combat;
			Asset->Nodes.Add(Node);
		}
		State->Kind = ELxAIBehaviorNodeKind::State;
		Phase->Kind = ELxAIBehaviorNodeKind::Phase;
		Action->Kind = ELxAIBehaviorNodeKind::Action;
		Action->Action = Index == 0 ? ELxAIBehaviorAction::PointPatrol : ELxAIBehaviorAction::Defend;
		Action->PointId = LxAITag_PointRoot;
		State->Children.Add(Phase->NodeId);
		Phase->Children.Add(Action->NodeId);
		States.Add(State);
	}
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* Entry = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
		Entry->Kind = ELxAIBehaviorNodeKind::Entry;
		Entry->NodeId = FGuid::NewGuid();
		Entry->Entry = static_cast<ELxAIBehaviorEntry>(Index);
		Entry->EntryPriority = Asset->GetDefaultEntryPriority(Entry->Entry);
		Entry->Children.Add(States[Index == 0 ? 0 : 1]->NodeId);
		Asset->Nodes.Add(Entry);
		Asset->Roots.Add(Entry->NodeId);
	}
	FText Error;
	if (!TestTrue(TEXT("追击测试资产有效"), Asset->ValidateConfiguration(Error))) { AddError(Error.ToString()); return false; }
	ULxAIControlAnalysis* Session = NewObject<ULxAIControlAnalysis>();
	Session->Initialize(Asset);
	const FVector Origin(12300, -8400, 500);
	Session->SetSelfLocation(Origin);
	TestEqual(TEXT("发现敌人进入战斗"), Session->Evaluate(Enemy, 10, 1, 0).StateId, States[1]->NodeId);
	Session->SetSelfLocation(Origin + FVector(6000, 0, 3000));
	Session->NotifyAttacked();
	TestEqual(TEXT("同一战斗受击不重置追击起点"), Session->Evaluate(Enemy, 1, 1, 0).StateId, States[1]->NodeId);
	Session->SetSelfLocation(Origin + FVector(9999, 0, 3000));
	TestEqual(TEXT("未达到水平追击距离继续战斗，更换敌人也不重置起点"), Session->Evaluate(OtherEnemy, 1, 1, 0).StateId, States[1]->NodeId);
	Session->SetSelfLocation(Origin + FVector(10000, 0, 0));
	TestEqual(TEXT("达到100米即退出战斗，不可打断不能绕过状态距离上限"), Session->Evaluate(OtherEnemy, 1, 1, 0, false).StateId, States[0]->NodeId);
	TestTrue(TEXT("进入返回保护"), Session->IsReturningFromChase());
	Session->SetSelfLocation(Origin + FVector(5000, 0, 0));
	Session->NotifyAttacked();
	TestEqual(TEXT("返回途中即使仍感知敌人或再次受击也保持巡逻"), Session->Evaluate(Enemy, 1, 1, 0).StateId, States[0]->NodeId);
	Session->CompleteChaseReturn();
	TestEqual(TEXT("抵达巡逻目的地后允许再次追击"), Session->Evaluate(Enemy, 10, 1, 0).StateId, States[1]->NodeId);
	Session->SetSelfLocation(Origin + FVector(14000, 0, 0));
	TestEqual(TEXT("再次追击使用新起点"), Session->Evaluate(Enemy, 10, 1, 0).StateId, States[1]->NodeId);
	States[1]->ChaseDistanceMeters = 0;
	Session->SetSelfLocation(Origin + FVector(100000, 0, 0));
	TestEqual(TEXT("距离设零取消限制"), Session->Evaluate(Enemy, 10, 1, 0).StateId, States[1]->NodeId);
	States[1]->ChaseDistanceMeters = -1;
	TestFalse(TEXT("不允许负追击距离"), States[1]->ValidateConfiguration(Error));
	return true;
}

#endif
