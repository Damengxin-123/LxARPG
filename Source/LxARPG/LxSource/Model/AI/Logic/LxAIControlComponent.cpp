#include "LxAIControlComponent.h"

ULxAIControlComponent::ULxAIControlComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BehaviorModule = CreateDefaultSubobject<ULxAIBehaviorModule>(TEXT("AI行为执行模块"));
}

void ULxAIControlComponent::BaseComponentInitialize()
{
	if (bControlInitialized)
	{
		return;
	}
	bControlInitialized = true;
	if (BehaviorModule)
	{
		BehaviorModule->InitializeModule(this);
	}
}

void ULxAIControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BehaviorModule)
	{
		BehaviorModule->ShutdownModule();
	}
	bControlInitialized = false;
	Super::EndPlay(EndPlayReason);
}

