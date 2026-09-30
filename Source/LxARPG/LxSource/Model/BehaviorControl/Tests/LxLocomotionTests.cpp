#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterLocomotionComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"

/** 验证按住冲刺、输入路由、属性加速、真实速度阈值及退出输入清理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxLocomotionSprintTest, "LxARPG.Locomotion.Sprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxLocomotionSprintTest::RunTest(const FString& Parameters)
{
	const auto SavedInputs = LxInputActionConfig::GetInputActionInfoMap();
	ON_SCOPE_EXIT
	{
		LxInputActionConfig::ClearInputActionConfig();
		for (const auto& Pair : SavedInputs) LxInputActionConfig::SetInputActionInfo(Pair.Value);
	};
	LxInputActionConfig::ClearInputActionConfig();
	LxInputActionConfig::EnsureDefaultSprintInputActionInfo();
	const auto* Sprint = LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Sprint);
	if (!TestNotNull(TEXT("自动补充冲刺输入"), Sprint)) return false;
	TestEqual(TEXT("默认左Shift"), Sprint->DefaultKey, EKeys::LeftShift);
	TestEqual(TEXT("冲刺按下松开成对"), Sprint->InteractionType, ELxInputInteractionType::PressAndRelease);
	FLxInputActionInfo Remapped = *Sprint;
	Remapped.DefaultKey = EKeys::RightShift;
	LxInputActionConfig::SetInputActionInfo(Remapped);
	LxInputActionConfig::EnsureDefaultSprintInputActionInfo();
	TestEqual(TEXT("保留用户改键"), LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Sprint)->DefaultKey, EKeys::RightShift);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Character = World->SpawnActor<ALxPlayerCharacter>();
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Player = NewObject<ULocalPlayer>(GEngine);
	Controller->Possess(Character);
	auto* Attributes = Character->GetCharacterAttributeComponent();
	Attributes->BaseComponentInitialize();
	auto* Values = Attributes->GetRuntimeAttributeSet();
	Values->FindMutableScalarAttribute(LxTag_Attribute_Action_BaseMovementSpeed)->Value = 6.0f;
	Values->FindMutableScalarAttribute(LxTag_Attribute_Action_MovementSpeedBonus)->Value = 0.0f;
	auto* Motion = Character->GetCharacterLocomotionComponent();
	Motion->BaseComponentInitialize();
	auto* Movement = Character->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Walking);
	auto* Inputs = Character->GetPlayerControlComponent()->GetMoveInputModule();
	Inputs->InitializeModule(Character->GetPlayerControlComponent());
	ON_SCOPE_EXIT { Inputs->ShutdownModule(); };
	FLxCharacterMotionSignal Signal;
	const auto Handle = Motion->OnBaseMotionSignalChanged.AddLambda([&](const FLxCharacterMotionSignal& InSignal) { Signal = InSignal; });
	ON_SCOPE_EXIT { Motion->OnBaseMotionSignalChanged.Remove(Handle); };
	/** 驱动物理采样入口，保留真实动画信号判定。 */
	auto Sample = [Movement, Motion](float Speed)
	{
		Movement->Velocity = FVector(Speed, 0, 0);
		Motion->RefreshBaseBehaviorState();
	};
	TestEqual(TEXT("默认低速300"), Movement->MaxWalkSpeed, 300.0f);
	Sample(300);
	TestEqual(TEXT("默认低速动画"), Signal.MotionType, ELxCharacterMotionType::Move);
	FLxInputValue Press;
	Press.m_blValue = true;
	LxInputActionConfig::SendInputEvent(ELxInputActionID::Sprint, Press, Controller);
	TestTrue(TEXT("输入分发到冲刺功能"), Motion->IsSprintRequested());
	TestEqual(TEXT("按住中速600"), Movement->MaxWalkSpeed, 600.0f);
	Sample(600);
	TestEqual(TEXT("冲刺中速动画"), Signal.MotionType, ELxCharacterMotionType::MediumMove);
	Values->FindMutableScalarAttribute(LxTag_Attribute_Action_MovementSpeedBonus)->Value = 1.0f;
	Attributes->RefreshCharacterMovementSpeed();
	TestEqual(TEXT("加成后中速1200"), Movement->MaxWalkSpeed, 1200.0f);
	TestEqual(TEXT("高速基准不随加成上涨"), Motion->GetHighSpeedThreshold(), 900.0f);
	Sample(900);
	TestEqual(TEXT("等于高速基准仍中速"), Signal.MotionType, ELxCharacterMotionType::MediumMove);
	Sample(901);
	TestEqual(TEXT("超出高速基准优先高速"), Signal.MotionType, ELxCharacterMotionType::Run);
	Sample(700);
	TestEqual(TEXT("降低速度恢复中速"), Signal.MotionType, ELxCharacterMotionType::MediumMove);
	FLxInputValue Release;
	Release.m_blValue = false;
	LxInputActionConfig::SendInputEvent(ELxInputActionID::Sprint, Release, Controller);
	TestFalse(TEXT("松开结束冲刺"), Motion->IsSprintRequested());
	TestEqual(TEXT("松开保留加成，恢复低速600"), Movement->MaxWalkSpeed, 600.0f);
	Sample(901);
	TestEqual(TEXT("低速档超速也播放高速"), Signal.MotionType, ELxCharacterMotionType::Run);
	Sample(0);
	TestEqual(TEXT("静止保持待机"), Signal.MotionType, ELxCharacterMotionType::Idle);
	Movement->SetMovementMode(MOVE_Falling);
	Sample(1200);
	TestEqual(TEXT("超速不覆盖滞空"), Signal.MotionType, ELxCharacterMotionType::Airborne);
	LxInputActionConfig::SendInputEvent(ELxInputActionID::Sprint, Press, Controller);
	Inputs->ShutdownModule();
	TestFalse(TEXT("输入模块关闭清除冲刺"), Motion->IsSprintRequested());
	return true;
}
#endif
