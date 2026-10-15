#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Model/Input/Logic/LxInputComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterLocomotionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"

namespace
{
	/** 临时替换输入表并在用例结束时恢复，避免向后续用例泄漏配置。 */
	struct FScopedInputTable
	{
		/** 用例开始前的完整默认配置。 */
		TMap<ELxInputActionID, FLxInputActionInfo> Original = LxInputActionConfig::GetInputActionInfoMap();
		/** 清空当前配置，所有测试行均由用例明确设置。 */
		FScopedInputTable() { LxInputActionConfig::ClearInputActionConfig(); }
		/** 恢复默认配置而不写入数据表资产。 */
		~FScopedInputTable()
		{
			LxInputActionConfig::ClearInputActionConfig();
			for (const auto& Pair : Original) LxInputActionConfig::SetInputActionInfo(Pair.Value);
		}
	};

	/** 创建明确指定键、轴向和触发方式的测试行。 */
	FLxInputActionInfo MakeInputRow(ELxInputActionID ID, const FKey& Key,
		EInputActionValueType ValueType = EInputActionValueType::Boolean)
	{
		FLxInputActionInfo Row;
		Row.InputActionID = ID;
		Row.DisplayName = FText::FromString(TEXT("输入映射测试"));
		Row.DefaultKey = Key;
		Row.ValueType = ValueType;
		Row.InteractionType = ELxInputInteractionType::PressAndRelease;
		Row.ValueMagnification = 1.0f;
		return Row;
	}

	/** 读取运行组件持有的真实映射，避免增加仅供测试使用的生产接口。 */
	UInputMappingContext* GetRuntimeMappingContext(ULxInputComponent* Input)
	{
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(ULxInputComponent::StaticClass(), TEXT("m_pDefaultMappingContext"));
		return Property ? Cast<UInputMappingContext>(Property->GetObjectPropertyValue_InContainer(Input)) : nullptr;
	}

	/** 从真实映射按业务输入行为查找条目。 */
	const FEnhancedActionKeyMapping* FindMapping(const UInputMappingContext* Context, ELxInputActionID ID)
	{
		if (!Context) return nullptr;
		return Context->GetMappings().FindByPredicate([ID](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action && LxInputActionConfig::GetInputActionIDByAction(Mapping.Action) == ID;
		});
	}
}

/** 验证可修改操作和物理按键过滤，保留键盘驱动的二维移动行为。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInputRemappableFilterTest, "LxARPG.Input.Settings.RemappableFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用独立默认行检查视角、连续轴、显式锁定和未配置行为。 */
bool FLxInputRemappableFilterTest::RunTest(const FString& Parameters)
{
	FScopedInputTable Table;
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::MoveForward, EKeys::W, EInputActionValueType::Axis2D));
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::LookX, EKeys::A, EInputActionValueType::Axis1D));
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::LookY, EKeys::B, EInputActionValueType::Axis1D));
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::ShortcutSwitch, EKeys::MouseWheelAxis, EInputActionValueType::Axis1D));
	FLxInputActionInfo Locked = MakeInputRow(ELxInputActionID::Aim, EKeys::RightMouseButton);
	Locked.bCanUserRemap = false;
	LxInputActionConfig::SetInputActionInfo(Locked);
	TestTrue(TEXT("键盘驱动的二维移动可以改键"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::MoveForward));
	TestFalse(TEXT("横向视角即使改为离散默认键仍不可改"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::LookX));
	TestFalse(TEXT("纵向视角不可改"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::LookY));
	TestFalse(TEXT("鼠标滚轮连续轴不可改"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::ShortcutSwitch));
	TestFalse(TEXT("配置表显式锁定的行为不可改"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::Aim));
	TestFalse(TEXT("未配置的行为不可改"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::Jump));
	TestFalse(TEXT("空行为不可改"), LxInputActionConfig::IsUserRemappableAction(ELxInputActionID::None));
	TestTrue(TEXT("支持键盘字母"), LxInputActionConfig::IsSupportedUserKey(EKeys::G));
	TestTrue(TEXT("支持键盘修饰键"), LxInputActionConfig::IsSupportedUserKey(EKeys::RightShift));
	TestTrue(TEXT("支持鼠标左右中键"), LxInputActionConfig::IsSupportedUserKey(EKeys::LeftMouseButton)
		&& LxInputActionConfig::IsSupportedUserKey(EKeys::RightMouseButton)
		&& LxInputActionConfig::IsSupportedUserKey(EKeys::MiddleMouseButton));
	TestFalse(TEXT("拒绝鼠标横向连续轴"), LxInputActionConfig::IsSupportedUserKey(EKeys::MouseX));
	TestFalse(TEXT("拒绝鼠标纵向连续轴"), LxInputActionConfig::IsSupportedUserKey(EKeys::MouseY));
	TestFalse(TEXT("拒绝鼠标滚轮连续轴"), LxInputActionConfig::IsSupportedUserKey(EKeys::MouseWheelAxis));
	TestFalse(TEXT("拒绝手柄离散键"), LxInputActionConfig::IsSupportedUserKey(EKeys::Gamepad_FaceButton_Bottom));
	TestFalse(TEXT("拒绝触摸输入"), LxInputActionConfig::IsSupportedUserKey(EKeys::TouchKeys[0]));
	TestFalse(TEXT("拒绝任意键占位符"), LxInputActionConfig::IsSupportedUserKey(EKeys::AnyKey));
	TestFalse(TEXT("保留菜单与取消用的Esc"), LxInputActionConfig::IsSupportedUserKey(EKeys::Escape));
	TestFalse(TEXT("拒绝无效键"), LxInputActionConfig::IsSupportedUserKey(FKey()));
	return true;
}

/** 验证真实增强输入组件的默认映射、重建去重和移动轴修饰器。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInputRuntimeMappingTest, "LxARPG.Input.Settings.RuntimeMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 仅初始化本地玩家的输入子系统，不初始化全局玩法子系统或访问玩家存档。 */
bool FLxInputRuntimeMappingTest::RunTest(const FString& Parameters)
{
	FScopedInputTable Table;
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::InteractionInteract, EKeys::F));
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::LookX, EKeys::MouseX, EInputActionValueType::Axis1D));
	FLxInputActionInfo Forward = MakeInputRow(ELxInputActionID::MoveForward, EKeys::W, EInputActionValueType::Axis2D);
	Forward.ValueDirection = ELxInputValueAxial::Y;
	Forward.ValueMagnification = -1.0f;
	LxInputActionConfig::SetInputActionInfo(Forward);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建输入映射隔离世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	World->SetGameInstance(Instance.Get());
	ON_SCOPE_EXIT { World->SetGameInstance(nullptr); };
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("创建真实输入宿主控制器"), Controller)) return false;
	// ULocalPlayer 的 ClassWithin 限制为 Engine，所属游戏世界由控制器和玩家视口关联。
	TStrongObjectPtr<ULocalPlayer> Local(NewObject<ULocalPlayer>(GEngine));
	Local->PlayerAdded(nullptr, 0);
	ON_SCOPE_EXIT { Local->PlayerRemoved(); };
	Controller->Player = Local.Get();
	Local->PlayerController = Controller;
	Controller->InputComponent = NewObject<UEnhancedInputComponent>(Controller);
	UEnhancedInputComponent* Enhanced = CastChecked<UEnhancedInputComponent>(Controller->InputComponent);
	ULxInputComponent* Input = NewObject<ULxInputComponent>(Controller);
	Input->RegisterComponent();
	ON_SCOPE_EXIT { Input->DestroyComponent(); };
	Input->BaseComponentInitialize();
	UInputMappingContext* Context = GetRuntimeMappingContext(Input);
	const FEnhancedActionKeyMapping* Initial = FindMapping(Context, ELxInputActionID::InteractionInteract);
	if (!TestNotNull(TEXT("生产输入组件创建交互映射"), Initial)) return false;
	TestEqual(TEXT("没有玩家设置子系统时使用默认F键"), Initial->Key, EKeys::F);
	const UInputAction* OriginalAction = Initial->Action;
	const int32 InitialBindingCount = Enhanced->GetActionEventBindings().Num();
	const int32 InitialMappingCount = Context->GetMappings().Num();
	// 使用真实角色的移动输入模块，检查映射更新是否同步结束已经按住的冲刺。
	ALxPlayerCharacter* Character = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建冲刺释放回归角色"), Character)) return false;
	Controller->Possess(Character);
	ULxCharacterLocomotionComponent* Motion = Character->GetCharacterLocomotionComponent();
	ULxPlayerMoveInputModule* MoveInput = Character->GetPlayerControlComponent()->GetMoveInputModule();
	Motion->BaseComponentInitialize();
	MoveInput->InitializeModule(Character->GetPlayerControlComponent());
	ON_SCOPE_EXIT { MoveInput->ShutdownModule(); };
	const FEnhancedActionKeyMapping* SprintMapping = FindMapping(Context, ELxInputActionID::Sprint);
	if (!TestNotNull(TEXT("生产输入组件保留冲刺映射"), SprintMapping)) return false;
	const FInputActionInstance SprintInstance(SprintMapping->Action);
	Input->HandlePressAndReleaseAction(SprintInstance, ETriggerEvent::Started);
	TestTrue(TEXT("真实按下事件经输入分发开启角色冲刺"), Motion->IsSprintRequested());
	// 模拟预设数据刷新后显式重建，确认重建过程复用既有行为与事件绑定。
	LxInputActionConfig::SetInputActionInfo(MakeInputRow(ELxInputActionID::InteractionInteract, EKeys::G));
	Input->RebuildInputMappings();
	TestFalse(TEXT("重建映射主动释放原按住事件，角色冲刺不会卡住"), Motion->IsSprintRequested());
	Input->HandlePressAndReleaseAction(SprintInstance, ETriggerEvent::Started);
	TestTrue(TEXT("映射重建后仍能重新按住冲刺"), Motion->IsSprintRequested());
	Input->ReleaseHeldInputActions();
	TestFalse(TEXT("显式释放按住输入同步结束角色冲刺"), Motion->IsSprintRequested());
	const FEnhancedActionKeyMapping* Updated = FindMapping(Context, ELxInputActionID::InteractionInteract);
	if (!TestNotNull(TEXT("重建后仍有交互映射"), Updated)) return false;
	TestEqual(TEXT("重建采用最新配置G键"), Updated->Key, EKeys::G);
	TestTrue(TEXT("重建复用原始输入行为对象"), Updated->Action == OriginalAction);
	Input->RebuildInputMappings();
	Input->BaseComponentInitialize();
	TestEqual(TEXT("重复重建映射不增加事件绑定"), Enhanced->GetActionEventBindings().Num(), InitialBindingCount);
	TestEqual(TEXT("重复重建映射不增加按键条目"), Context->GetMappings().Num(), InitialMappingCount);
	const FEnhancedActionKeyMapping* Look = FindMapping(Context, ELxInputActionID::LookX);
	if (TestNotNull(TEXT("视角轴映射被保留"), Look)) TestEqual(TEXT("视角仍使用鼠标连续轴"), Look->Key, EKeys::MouseX);
	const FEnhancedActionKeyMapping* Movement = FindMapping(Context, ELxInputActionID::MoveForward);
	if (!TestNotNull(TEXT("重建后保留移动映射"), Movement)) return false;
	TestEqual(TEXT("保留移动的轴向修饰器数量"), Movement->Modifiers.Num(), 2);
	if (Movement->Modifiers.Num() == 2)
	{
		TestNotNull(TEXT("保留二维轴向交换修饰器"), Cast<UInputModifierSwizzleAxis>(Movement->Modifiers[0]));
		const UInputModifierScalar* Scalar = Cast<UInputModifierScalar>(Movement->Modifiers[1]);
		if (TestNotNull(TEXT("保留倍率修饰器"), Scalar)) TestEqual(TEXT("保留负方向倍率"), Scalar->Scalar.Y, -1.0);
	}
	return true;
}

#endif