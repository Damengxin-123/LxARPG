#include "LxWaitMechanismStateChangedAsyncAction.h"

#include "LxInteractableComponent.h"
#include "LxTriggerMechanismInteractionComponent.h"

ULxWaitMechanismStateChangedAsyncAction* ULxWaitMechanismStateChangedAsyncAction::WaitForMechanismStateChanged(
	ULxInteractableComponent* InInteractableComponent)
{
	ULxWaitMechanismStateChangedAsyncAction* Action = NewObject<ULxWaitMechanismStateChangedAsyncAction>();
	if (!Action)
	{
		return nullptr;
	}

	Action->InteractableComponent = InInteractableComponent;
	if (IsValid(InInteractableComponent))
	{
		Action->RegisterWithGameInstance(InInteractableComponent);
	}
	return Action;
}

void ULxWaitMechanismStateChangedAsyncAction::Activate()
{
	if (!IsValid(InteractableComponent))
	{
		FinishAsCancelled(LastKnownState);
		return;
	}

	// 简单机关通常只有一个机关功能模块，记录其当前状态供组件结束时输出。
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractableComponent->GetInteractionFeatures())
	{
		if (const ULxTriggerMechanismInteractionComponent* MechanismFeature =
			Cast<ULxTriggerMechanismInteractionComponent>(InteractionFeature))
		{
			LastKnownState = MechanismFeature->GetMechanismState();
			break;
		}
	}

	InteractableComponent->OnMechanismStateChanged.AddUniqueDynamic(
		this, &ULxWaitMechanismStateChangedAsyncAction::HandleMechanismStateChanged);
	InteractableComponent->OnInteractableComponentEndPlayNative.AddUObject(
		this, &ULxWaitMechanismStateChangedAsyncAction::HandleInteractableComponentEndPlay);
}

void ULxWaitMechanismStateChangedAsyncAction::HandleMechanismStateChanged(ELxMechanismState NewState)
{
	if (bFinished)
	{
		return;
	}
	LastKnownState = NewState;
	StateChanged.Broadcast(NewState);
}

void ULxWaitMechanismStateChangedAsyncAction::HandleInteractableComponentEndPlay()
{
	FinishAsCancelled(LastKnownState);
}

void ULxWaitMechanismStateChangedAsyncAction::FinishAsCancelled(ELxMechanismState LastState)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (IsValid(InteractableComponent))
	{
		InteractableComponent->OnMechanismStateChanged.RemoveDynamic(
			this, &ULxWaitMechanismStateChangedAsyncAction::HandleMechanismStateChanged);
		InteractableComponent->OnInteractableComponentEndPlayNative.RemoveAll(this);
	}

	InteractableComponent = nullptr;
	Cancelled.Broadcast(LastState);
	SetReadyToDestroy();
}
