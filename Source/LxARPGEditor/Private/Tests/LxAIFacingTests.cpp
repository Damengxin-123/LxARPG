#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorTreeExecutor.h"

/** 验证AI只有一个旋转来源，移动朝向稳定，战斗朝向请求能正确释放。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIFacingTest, "LxARPG.AIControlConfig.FacingOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIFacingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	ALxAICharacter* Character = World->SpawnActor<ALxAICharacter>();
	AAIController* Controller = World->SpawnActor<AAIController>();
	if (!TestNotNull(TEXT("测试角色"), Character) || !TestNotNull(TEXT("测试控制器"), Controller)) return false;
	Controller->Possess(Character);
	ULxCharacterBehaviorControlComponent* Behavior = Character->GetCharacterBehaviorControlComponent();
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	Character->bUseControllerRotationYaw = true;
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = true;
	Movement->Velocity = FVector(300, 0, 0);
	Character->SetActorRotation(FRotator(0, 90, 0));
	float PreviousError = 90.0f;
	for (int32 Frame = 0; Frame < 10; ++Frame)
	{
		Behavior->TickComponent(0.02f, LEVELTICK_All, nullptr);
		Controller->SetControlRotation(FRotator(0, 180, 0));
		Character->FaceRotation(Controller->GetControlRotation(), 0.02f);
		const float Error = FMath::Abs(FMath::FindDeltaAngleDegrees(Character->GetActorRotation().Yaw, 0.0f));
		TestTrue(TEXT("移动朝向单调接近速度方向，不被控制器反向拉回"), Error <= PreviousError);
		PreviousError = Error;
	}
	TestFalse(TEXT("关闭控制器直接旋转角色"), Character->bUseControllerRotationYaw);
	TestFalse(TEXT("关闭移动组件的第二套旋转"), Movement->bOrientRotationToMovement || Movement->bUseControllerDesiredRotation);
	Movement->Velocity = FVector::ZeroVector;
	const FRotator StoppedRotation = Character->GetActorRotation();
	Behavior->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("停止移动后保持朝向"), Character->GetActorRotation().Equals(StoppedRotation));

	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	ULxAIBehaviorTreeNodeData* Action = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	Phase->NodeId = FGuid::NewGuid();
	Phase->Kind = ELxAIBehaviorNodeKind::Phase;
	Action->NodeId = FGuid::NewGuid();
	Action->Kind = ELxAIBehaviorNodeKind::Action;
	Action->Action = ELxAIBehaviorAction::Alert;
	Phase->Children.Add(Action->NodeId);
	Asset->Nodes.Add(Phase);
	Asset->Nodes.Add(Action);
	FLxAIAnalysisDecision Decision;
	Decision.bHasBranch = true;
	Decision.PhaseId = Phase->NodeId;
	ULxAIBehaviorTreeExecutor* Executor = NewObject<ULxAIBehaviorTreeExecutor>(Character);
	Executor->Initialize(Character);
	AActor* Enemy = World->SpawnActor<AActor>();
	Executor->TickExecution(Asset, Decision, Enemy, Character->GetActorLocation() + FVector(-400, 0, 0), 0.1f);
	TestTrue(TEXT("警戒持有面向敌人的请求"), Behavior->IsFacingControlActive());
	Executor->ResetExecution();
	TestFalse(TEXT("退出战斗立即释放朝向，不残留延迟回头"), Behavior->IsFacingControlActive());
	Action->Action = ELxAIBehaviorAction::RouteFlee;
	Executor->TickExecution(Asset, Decision, Enemy, FVector(-400, 0, 0), 0.1f);
	TestFalse(TEXT("逃跑不会要求面向身后的敌人"), Behavior->IsFacingControlActive());
	Action->Action = ELxAIBehaviorAction::Alert;
	Executor->ResetExecution();
	Executor->TickExecution(Asset, Decision, Enemy, Character->GetActorLocation() + FVector(-400, 0, 0), 0.1f);
	Behavior->AddFacingControlRequest();
	Executor->ResetExecution();
	TestTrue(TEXT("释放行为树请求不会清除技能等外部朝向请求"), Behavior->IsFacingControlActive());
	Behavior->RemoveFacingControlRequest(0.0f);
	Controller->UnPossess();
	Behavior->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("离开AI控制时恢复原旋转设置"), Character->bUseControllerRotationYaw && Movement->bOrientRotationToMovement && Movement->bUseControllerDesiredRotation);
	return true;
}

#endif
