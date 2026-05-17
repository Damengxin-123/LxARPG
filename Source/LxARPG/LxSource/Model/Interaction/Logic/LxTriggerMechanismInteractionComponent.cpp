#include "LxTriggerMechanismInteractionComponent.h"

ULxTriggerMechanismInteractionComponent::ULxTriggerMechanismInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::TriggerMechanism;
}

bool ULxTriggerMechanismInteractionComponent::TriggerMechanism_Implementation(
	ULxPlayerInteractionComponent* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	switch (MechanismState)
	{
	case ELxMechanismState::Closed:
		SetMechanismState(ELxMechanismState::Opened);
		return true;
	case ELxMechanismState::Opened:
		SetMechanismState(ELxMechanismState::Closed);
		return true;
	case ELxMechanismState::CannotOpen:
	default:
		return false;
	}
}

void ULxTriggerMechanismInteractionComponent::SetMechanismState(ELxMechanismState InMechanismState)
{
	if (MechanismState == InMechanismState)
	{
		return;
	}

	MechanismState = InMechanismState;
	OnMechanismStateChanged.Broadcast(MechanismState);
	OnDataChange.Broadcast();
}

FGameplayTag ULxTriggerMechanismInteractionComponent::GetPromptTextTag() const
{
	if (const FGameplayTag* PromptTextTagByState = MechanismStatePromptTextTags.Find(MechanismState))
	{
		return *PromptTextTagByState;
	}

	return Super::GetPromptTextTag();
}

bool ULxTriggerMechanismInteractionComponent::ExecuteInteraction_Implementation(
	ULxPlayerInteractionComponent* PlayerInteractionComponent)
{
	return TriggerMechanism(PlayerInteractionComponent);
}
