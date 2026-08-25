#include "LxPlayerControlComponent.h"

ULxPlayerControlComponent::ULxPlayerControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	MoveInputModule = CreateDefaultSubobject<ULxPlayerMoveInputModule>(TEXT("玩家移动输入模块"));
	AimModule = CreateDefaultSubobject<ULxPlayerAimModule>(TEXT("玩家瞄准模块"));
	InteractionModule = CreateDefaultSubobject<ULxPlayerInteractionModule>(TEXT("玩家交互模块"));
}

void ULxPlayerControlComponent::BaseComponentInitialize()
{
	// 角色 BeginPlay 时可能尚未被玩家控制器占有；OnPossess 后必须允许再次刷新模块输入监听。
	// 各模块的输入注册本身会去重，因此重复初始化不会产生重复回调。
	bControlInitialized = true;

	if (MoveInputModule) MoveInputModule->InitializeModule(this);
	if (AimModule) AimModule->InitializeModule(this);
	if (InteractionModule) InteractionModule->InitializeModule(this);
}

void ULxPlayerControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionModule) InteractionModule->ShutdownModule();
	if (AimModule) AimModule->ShutdownModule();
	if (MoveInputModule) MoveInputModule->ShutdownModule();
	bControlInitialized = false;
	Super::EndPlay(EndPlayReason);
}

void ULxPlayerControlComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (AimModule)
	{
		AimModule->TickModule(DeltaTime);
	}
}
