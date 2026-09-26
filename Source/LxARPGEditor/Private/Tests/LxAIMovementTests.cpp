#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/AI/Logic/LxAIBehaviorTreeExecutor.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include <limits>

/** 验证默认标识、属性加成顺序、执行器切换和退出后的倍率恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAIMovementTest, "LxARPG.AIControlConfig.Movement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAIMovementTest::RunTest(const FString& Parameters)
{
	ULxAIBehaviorTreeNodeData* Defaults = NewObject<ULxAIBehaviorTreeNodeData>();
	Defaults->Action = ELxAIBehaviorAction::PointPatrol;
	TestEqual(TEXT("巡逻默认低速"), Defaults->GetMotionType(), ELxCharacterMotionType::Move);
	Defaults->Action = ELxAIBehaviorAction::RandomFlee;
	TestEqual(TEXT("逃跑默认高速"), Defaults->GetMotionType(), ELxCharacterMotionType::Run);
	Defaults->MotionType = ELxCharacterMotionType::MediumMove;
	TestEqual(TEXT("自定义类型优先"), Defaults->GetMotionType(), ELxCharacterMotionType::MediumMove);
	FLxAIMovementConfig Config;
	FText Error;
	TestTrue(TEXT("默认倍率有效"), Config.ValidateConfiguration(Error));
	Config.LowSpeedMultiplier = -1.0f;
	TestFalse(TEXT("拒绝负倍率"), Config.ValidateConfiguration(Error));
	Config.LowSpeedMultiplier = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("拒绝非有限倍率"), Config.ValidateConfiguration(Error));

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	ALxAICharacter* Character = World->SpawnActor<ALxAICharacter>();
	if (!TestNotNull(TEXT("测试角色"), Character)) return false;
	ULxCharacterAttributeComponent* Attributes = Character->GetCharacterAttributeComponent();
	Attributes->BaseComponentInitialize();
	ULxCharacterBaseAttributeSet* Values = Attributes->GetRuntimeAttributeSet();
	if (!TestNotNull(TEXT("运行时属性"), Values)) return false;
	Values->FindMutableScalarAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.行动.基础移动速度")))->Value = 6.0f;
	Values->FindMutableScalarAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.行动.移动速度加成")))->Value = 0.2f;
	ULxCharacterBehaviorControlComponent* Behavior = Character->GetCharacterBehaviorControlComponent();
	Behavior->SetBehaviorMotion(ELxCharacterMotionType::Move, 0.5f);
	TestEqual(TEXT("6米基础速度加成20%后低速为360厘米"), Character->GetCharacterMovement()->MaxWalkSpeed, 360.0f);
	Behavior->SetBehaviorMotion(ELxCharacterMotionType::Run, 1.5f);
	TestEqual(TEXT("高速为1080厘米"), Character->GetCharacterMovement()->MaxWalkSpeed, 1080.0f);
	Attributes->RefreshCharacterMovementSpeed();
	TestEqual(TEXT("重复刷新不会累乘"), Character->GetCharacterMovement()->MaxWalkSpeed, 1080.0f);
	Values->FindMutableScalarAttribute(FGameplayTag::RequestGameplayTag(TEXT("属性.行动.移动速度加成")))->Value = 0.5f;
	Attributes->RefreshCharacterMovementSpeed();
	TestEqual(TEXT("属性变更保留当前运动倍率"), Character->GetCharacterMovement()->MaxWalkSpeed, 1350.0f);

	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>();
	ULxAIBehaviorTreeNodeData* Phase = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	ULxAIBehaviorTreeNodeData* Action = NewObject<ULxAIBehaviorTreeNodeData>(Asset);
	Phase->Kind = ELxAIBehaviorNodeKind::Phase;
	Phase->NodeId = FGuid::NewGuid();
	Action->Kind = ELxAIBehaviorNodeKind::Action;
	Action->NodeId = FGuid::NewGuid();
	Action->WaitSeconds = 0.0f;
	Action->MotionType = ELxCharacterMotionType::Move;
	Phase->Children.Add(Action->NodeId);
	Asset->Nodes.Add(Phase);
	Asset->Nodes.Add(Action);
	FLxAIAnalysisDecision Decision;
	Decision.bHasBranch = true;
	Decision.PhaseId = Phase->NodeId;
	ULxAIBehaviorTreeExecutor* Executor = NewObject<ULxAIBehaviorTreeExecutor>(Character);
	Executor->Initialize(Character);
	Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestEqual(TEXT("执行器发布统一类型"), Behavior->GetCurrentMotionType(), ELxCharacterMotionType::Move);
	TestEqual(TEXT("执行器应用树倍率"), Character->GetCharacterMovement()->MaxWalkSpeed, 450.0f);
	Action->MotionType = ELxCharacterMotionType::MediumMove;
	Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	TestEqual(TEXT("相同叶节点更新运动档位"), Character->GetCharacterMovement()->MaxWalkSpeed, 900.0f);
	Action->MotionType = ELxCharacterMotionType::Run;
	Executor->TickExecution(Asset, Decision, nullptr, FVector::ZeroVector, 0.1f);
	Executor->ResetExecution();
	TestEqual(TEXT("退出行为树恢复属性速度"), Character->GetCharacterMovement()->MaxWalkSpeed, 900.0f);
	return true;
}
#endif
