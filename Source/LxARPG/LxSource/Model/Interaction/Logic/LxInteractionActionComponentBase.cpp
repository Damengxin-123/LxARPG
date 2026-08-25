#include "LxInteractionActionComponentBase.h"

FGameplayTag ULxInteractionActionComponentBase::GetPromptTextTag() const
{
	return PromptTextTag;
}

void ULxInteractionActionComponentBase::SetPromptTextTag(FGameplayTag InPromptTextTag)
{
	PromptTextTag = InPromptTextTag;
	OnDataChange.Broadcast();
}

void ULxInteractionActionComponentBase::SetInteractionState(ELxInteractionDataState InState)
{
	if (InteractionState == InState)
	{
		return;
	}

	InteractionState = InState;
	OnInteractionStateChanged.Broadcast(InteractionState);
	OnDataChange.Broadcast();
}

bool ULxInteractionActionComponentBase::IsInteractionValid_Implementation() const
{
	return InteractionState == ELxInteractionDataState::Interactable;
}

bool ULxInteractionActionComponentBase::CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	return PlayerInteractionComponent != nullptr;
}

bool ULxInteractionActionComponentBase::ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	return IsInteractionValid() && CheckInteractionRequirement(PlayerInteractionComponent);
}
