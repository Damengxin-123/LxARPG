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
	if (bActivated || bFinished)
	{
		return;
	}
	bActivated = true;

	if (!IsValid(InteractableComponent))
	{
		FinishAsCancelled(LastKnownState);
		return;
	}

	InteractableComponent->OnMechanismStateChanged.AddUObject(
		this, &ULxWaitMechanismStateChangedAsyncAction::HandleMechanismStateChanged);
	InteractableComponent->OnInteractableComponentEndPlayNative.AddUObject(
		this, &ULxWaitMechanismStateChangedAsyncAction::HandleInteractableComponentEndPlay);

	// 读档可能早于蓝图开始监听；绑定完成后补发当前状态，使门、开关等表现立即同步。
	for (ULxInteractionActionComponentBase* InteractionFeature : InteractableComponent->GetInteractionFeatures())
	{
		if (const ULxTriggerMechanismInteractionComponent* MechanismFeature =
			Cast<ULxTriggerMechanismInteractionComponent>(InteractionFeature))
		{
			HandleMechanismStateChanged(MechanismFeature->GetMechanismState());
			break;
		}
	}
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
		InteractableComponent->OnMechanismStateChanged.RemoveAll(this);
		InteractableComponent->OnInteractableComponentEndPlayNative.RemoveAll(this);
	}

	InteractableComponent = nullptr;
	Cancelled.Broadcast(LastState);
	SetReadyToDestroy();
}
