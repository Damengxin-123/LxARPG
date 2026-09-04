#include "LxFunctionPageInteractionComponent.h"

ULxFunctionPageInteractionComponent::ULxFunctionPageInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::FunctionPage;
	bOpenFunctionUI = true;
}

void ULxFunctionPageInteractionComponent::ApplyConfig(const FLxFunctionPageInteractionConfig& InConfig)
{
	FunctionPageID = InConfig.FunctionPageID;
}

bool ULxFunctionPageInteractionComponent::ExecuteInteraction_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	SetInteractionState(ELxInteractionDataState::Interacting);
	return true;
}
