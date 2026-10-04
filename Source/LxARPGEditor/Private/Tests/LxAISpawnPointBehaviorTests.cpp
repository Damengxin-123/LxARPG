#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EdGraph/EdGraphPin.h"
#include "LxAIBehaviorTreeAssetFactory.h"
#include "LxAIBehaviorTreeEdGraph.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorTreeExecutor.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/World/AISpawn/LxAISpawnPointActor.h"

/** 验证刷怪点行为不需要场景标签，且在对应阶段菜单中能够直接创建。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointBehaviorConfigurationTest,
	"LxARPG.AISpawnPoint.BehaviorConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAISpawnPointBehaviorConfigurationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("新增行为保留旧死亡行为的序列化值"), static_cast<uint8>(ELxAIBehaviorAction::EnterDeath), static_cast<uint8>(11));
	ULxAIBehaviorTreeAssetFactory* Factory = NewObject<ULxAIBehaviorTreeAssetFactory>();
	ULxAIBehaviorTreeAsset* Asset = Cast<ULxAIBehaviorTreeAsset>(Factory->FactoryCreateNew(
		ULxAIBehaviorTreeAsset::StaticClass(), GetTransientPackage(), NAME_None, RF_Transactional, nullptr, GWarn));
	ULxAIBehaviorTreeEdGraph* Graph = Asset ? Cast<ULxAIBehaviorTreeEdGraph>(Asset->EditorGraph) : nullptr;
	if (!TestNotNull(TEXT("创建行为图"), Graph)) return false;
	for (const ELxAIBehaviorAction Action : {ELxAIBehaviorAction::SpawnPointPatrol, ELxAIBehaviorAction::SpawnPointFlee})
	{
		const ELxAIBehaviorState ExpectedState = Action == ELxAIBehaviorAction::SpawnPointPatrol
			? ELxAIBehaviorState::Patrol : ELxAIBehaviorState::Flee;
		ULxAIBehaviorTreeEdGraphNode* Phase = nullptr;
		for (UEdGraphNode* RawNode : Graph->Nodes)
		{
			ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
			if (Node && Node->Data && Node->Data->Kind == ELxAIBehaviorNodeKind::Phase && Node->Data->State == ExpectedState)
			{
				Phase = Node;
				break;
			}
		}
		if (!TestNotNull(TEXT("找到对应分类的阶段"), Phase)) return false;
		FGraphContextMenuBuilder Menu(Graph);
		UEdGraphPin* OutputPin = nullptr;
		for (UEdGraphPin* Pin : Phase->Pins)
			if (Pin && Pin->Direction == EGPD_Output) OutputPin = Pin;
		if (!TestNotNull(TEXT("找到阶段输出引脚"), OutputPin)) return false;
		Menu.FromPin = OutputPin;
		Graph->GetSchema()->GetGraphContextActions(Menu);
		TSharedPtr<FLxAIBehaviorTreeNewNodeAction> Creation;
		for (int32 Index = 0; Index < Menu.GetNumActions(); ++Index)
		{
			const TSharedPtr<FLxAIBehaviorTreeNewNodeAction> Candidate =
				StaticCastSharedPtr<FLxAIBehaviorTreeNewNodeAction>(Menu.GetSchemaAction(Index));
			if (Candidate.IsValid() && Candidate->Kind == ELxAIBehaviorNodeKind::Action && Candidate->Action == Action)
				Creation = Candidate;
		}
		if (!TestTrue(TEXT("阶段菜单提供刷怪点行为"), Creation.IsValid())) return false;
		ULxAIBehaviorTreeEdGraphNode* Added = Cast<ULxAIBehaviorTreeEdGraphNode>(
			Creation->PerformAction(Graph, OutputPin, FVector2f(1400, 0), false));
		if (!TestNotNull(TEXT("菜单创建刷怪点行为节点"), Added)) return false;
		TestTrue(TEXT("行为自动连接到正确阶段"), Phase->Data->Children.Contains(Added->Data->NodeId));
		TestEqual(TEXT("行为具有正确状态分类"), Added->Data->GetState(), ExpectedState);
		TestEqual(TEXT("巡逻与逃跑使用各自默认运动类型"), Added->Data->GetMotionType(),
			Action == ELxAIBehaviorAction::SpawnPointPatrol ? ELxCharacterMotionType::Move : ELxCharacterMotionType::Run);
		TestFalse(TEXT("节点不需要点位标签"), Added->Data->PointId.IsValid());
		FText Error;
		TestTrue(TEXT("未绑定任何场景实例时资产节点仍有效"), Added->Data->ValidateConfiguration(Error));
		if (Action == ELxAIBehaviorAction::SpawnPointPatrol)
		{
			Added->Data->WaitSeconds = -1.0f;
			TestFalse(TEXT("刷怪点巡逻拒绝负数等待时间"), Added->Data->ValidateConfiguration(Error));
		}
	}
	return true;
}

/** 验证生成前注入、共享资产的角色隔离，以及未绑定或销毁刷怪点时的实际行为回退。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointBehaviorBindingTest,
	"LxARPG.AISpawnPoint.BehaviorBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAISpawnPointBehaviorBindingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	ALxAISpawnPointActor* FirstPoint = World->SpawnActor<ALxAISpawnPointActor>();
	ALxAISpawnPointActor* SecondPoint = World->SpawnActor<ALxAISpawnPointActor>();
	if (!TestNotNull(TEXT("创建第一个刷怪点"), FirstPoint) || !TestNotNull(TEXT("创建第二个刷怪点"), SecondPoint)) return false;
	SecondPoint->SetActorLocation(FVector(1000000.0, 0.0, 0.0));
	const FTransform FirstTransform(FirstPoint->GetWorldCenter());
	const FTransform SecondTransform(SecondPoint->GetWorldCenter());
	ALxAICharacter* FirstCharacter = World->SpawnActorDeferred<ALxAICharacter>(
		ALxAICharacter::StaticClass(), FirstTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	ALxAICharacter* SecondCharacter = World->SpawnActorDeferred<ALxAICharacter>(
		ALxAICharacter::StaticClass(), SecondTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("延迟生成第一个角色"), FirstCharacter) || !TestNotNull(TEXT("延迟生成第二个角色"), SecondCharacter)) return false;
	FirstCharacter->SetSpawnPoint(FirstPoint);
	SecondCharacter->SetSpawnPoint(SecondPoint);
	TestEqual(TEXT("完成生成前即可读取刷怪点"), FirstCharacter->GetSpawnPoint(), FirstPoint);
	FirstCharacter->FinishSpawning(FirstTransform);
	SecondCharacter->FinishSpawning(SecondTransform);
	TestEqual(TEXT("完成生成后保留所属刷怪点"), SecondCharacter->GetSpawnPoint(), SecondPoint);

	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	ULxAIBehaviorTreeNodeData* ReturnAction = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	ULxAIBehaviorTreeNodeData* Fallback = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	for (ULxAIBehaviorTreeNodeData* Node : {Phase, ReturnAction, Fallback})
	{
		Node->NodeId = FGuid::NewGuid();
		Asset->Nodes.Add(Node);
	}
	Phase->Kind = ELxAIBehaviorNodeKind::Phase;
	ReturnAction->Kind = Fallback->Kind = ELxAIBehaviorNodeKind::Action;
	ReturnAction->Action = ELxAIBehaviorAction::SpawnPointFlee;
	Fallback->Action = ELxAIBehaviorAction::Wait;
	Fallback->WaitSeconds = 0.0f;
	Fallback->bCanInterrupt = false;
	Fallback->Order = 1;
	Phase->Children = {ReturnAction->NodeId, Fallback->NodeId};
	FLxAIAnalysisDecision Decision;
	Decision.bHasBranch = true;
	Decision.Entry = ELxAIBehaviorEntry::Attacked;
	Decision.PhaseId = Phase->NodeId;
	ULxAIBehaviorTreeExecutor* FirstExecutor = NewObject<ULxAIBehaviorTreeExecutor>(FirstCharacter);
	ULxAIBehaviorTreeExecutor* SecondExecutor = NewObject<ULxAIBehaviorTreeExecutor>(SecondCharacter);
	FirstExecutor->Initialize(FirstCharacter);
	SecondExecutor->Initialize(SecondCharacter);
	FirstExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	SecondExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	// 正常完成返回行为时，只切换到下一叶；失败时则在同帧执行待机并持有执行锁。
	TestTrue(TEXT("第一个角色在自己的刷怪点完成返回"), FirstExecutor->CanInterruptCurrentAction());
	TestTrue(TEXT("共享资产的第二个角色在不同刷怪点独立完成返回"), SecondExecutor->CanInterruptCurrentAction());
	TestEqual(TEXT("完成返回后切换到下一叶"), SecondExecutor->GetCurrentActionNodeId(), Fallback->NodeId);
	TestFalse(TEXT("共享行为资产没有被写入场景点位ID"), ReturnAction->PointId.IsValid());

	FirstCharacter->SetSpawnPoint(SecondPoint);
	FirstExecutor->ResetExecution();
	FirstExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestFalse(TEXT("更换为远处刷怪点后不会错误地视为已到达"), FirstExecutor->CanInterruptCurrentAction());
	FirstCharacter->SetSpawnPoint(nullptr);
	FirstExecutor->ResetExecution();
	FirstExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestFalse(TEXT("未绑定刷怪点时同轮执行后续行为"), FirstExecutor->CanInterruptCurrentAction());
	ReturnAction->Action = ELxAIBehaviorAction::SpawnPointPatrol;
	FirstExecutor->ResetExecution();
	FirstExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestFalse(TEXT("未绑定刷怪点时巡逻也能安全回退"), FirstExecutor->CanInterruptCurrentAction());
	FirstCharacter->SetSpawnPoint(FirstPoint);
	FirstExecutor->ResetExecution();
	FirstExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestFalse(TEXT("刷怪点内没有导航网格时巡逻不会卡住后续行为"), FirstExecutor->CanInterruptCurrentAction());
	SecondPoint->Destroy();
	TestNull(TEXT("刷怪点销毁后角色弱引用自动失效"), SecondCharacter->GetSpawnPoint());
	ReturnAction->Action = ELxAIBehaviorAction::SpawnPointFlee;
	SecondExecutor->ResetExecution();
	SecondExecutor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestFalse(TEXT("所属刷怪点销毁后返回行为安全回退"), SecondExecutor->CanInterruptCurrentAction());
	return true;
}

#endif
