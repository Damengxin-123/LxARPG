#include "LxWaitItemTransferCompletedAsyncAction.h"

#include "LxInteractableComponent.h"

ULxWaitItemTransferCompletedAsyncAction* ULxWaitItemTransferCompletedAsyncAction::WaitForItemTransferCompleted(
	ULxInteractableComponent* InInteractableComponent)
{
	ULxWaitItemTransferCompletedAsyncAction* Action = NewObject<ULxWaitItemTransferCompletedAsyncAction>();
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

void ULxWaitItemTransferCompletedAsyncAction::Activate()
{
	if (!IsValid(InteractableComponent))
	{
		FinishWaiting(true);
		return;
	}

	if (!InteractableComponent->OnItemTransferCompleted.IsBoundToObject(this))
	{
		InteractableComponent->OnItemTransferCompleted.AddUObject(this, &ULxWaitItemTransferCompletedAsyncAction::HandleItemTransferCompleted);
	}
	InteractableComponent->OnInteractableComponentEndPlayNative.AddUObject(
		this, &ULxWaitItemTransferCompletedAsyncAction::HandleInteractableComponentEndPlay);
}

void ULxWaitItemTransferCompletedAsyncAction::HandleItemTransferCompleted()
{
	FinishWaiting(false);
}

void ULxWaitItemTransferCompletedAsyncAction::HandleInteractableComponentEndPlay()
{
	FinishWaiting(true);
}

void ULxWaitItemTransferCompletedAsyncAction::FinishWaiting(bool bWasCancelled)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (IsValid(InteractableComponent))
	{
		InteractableComponent->OnItemTransferCompleted.RemoveAll(this);
		InteractableComponent->OnInteractableComponentEndPlayNative.RemoveAll(this);
	}

	InteractableComponent = nullptr;
	if (bWasCancelled)
	{
		Cancelled.Broadcast();
	}
	else
	{
		Completed.Broadcast();
	}
	SetReadyToDestroy();
}
