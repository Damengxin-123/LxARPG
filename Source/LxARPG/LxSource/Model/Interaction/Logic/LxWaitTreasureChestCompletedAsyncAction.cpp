#include "LxWaitTreasureChestCompletedAsyncAction.h"

#include "LxInteractableComponent.h"

ULxWaitTreasureChestCompletedAsyncAction* ULxWaitTreasureChestCompletedAsyncAction::WaitForTreasureChestCompleted(
	ULxInteractableComponent* InInteractableComponent)
{
	ULxWaitTreasureChestCompletedAsyncAction* Action = NewObject<ULxWaitTreasureChestCompletedAsyncAction>();
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

void ULxWaitTreasureChestCompletedAsyncAction::Activate()
{
	if (!IsValid(InteractableComponent))
	{
		FinishWaiting(true);
		return;
	}

	InteractableComponent->OnTreasureChestCompleted.AddUniqueDynamic(
		this, &ULxWaitTreasureChestCompletedAsyncAction::HandleTreasureChestCompleted);
	InteractableComponent->OnInteractableComponentEndPlayNative.AddUObject(
		this, &ULxWaitTreasureChestCompletedAsyncAction::HandleInteractableComponentEndPlay);
}

void ULxWaitTreasureChestCompletedAsyncAction::HandleTreasureChestCompleted()
{
	FinishWaiting(false);
}

void ULxWaitTreasureChestCompletedAsyncAction::HandleInteractableComponentEndPlay()
{
	FinishWaiting(true);
}

void ULxWaitTreasureChestCompletedAsyncAction::FinishWaiting(bool bWasCancelled)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (IsValid(InteractableComponent))
	{
		InteractableComponent->OnTreasureChestCompleted.RemoveDynamic(
			this, &ULxWaitTreasureChestCompletedAsyncAction::HandleTreasureChestCompleted);
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
