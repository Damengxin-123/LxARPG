#include "LxFunctionPageInteractionComponent.h"

#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "GameFramework/PlayerController.h"

ULxFunctionPageInteractionComponent::ULxFunctionPageInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::FunctionPage;
	bOpenFunctionUI = true;
}

void ULxFunctionPageInteractionComponent::ApplyConfig(const FLxFunctionPageInteractionConfig& InConfig)
{
	FunctionPageID = InConfig.FunctionPageID;
	PageWidgetClass = InConfig.PageWidgetClass;
}

bool ULxFunctionPageInteractionComponent::CheckInteractionRequirement_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	return Super::CheckInteractionRequirement_Implementation(PlayerInteractionComponent)
		&& PageWidgetClass && !PageWidgetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated);
}

bool ULxFunctionPageInteractionComponent::ExecuteInteraction_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	const ALxPlayerCharacter* Character = PlayerInteractionComponent->GetPlayerCharacter();
	const APlayerController* Controller = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	ULxLocalPlayerSubsystem* Subsystem = Controller
		? ULxLocalPlayerSubsystem::GetFromLocalPlayer(Controller->GetLocalPlayer()) : nullptr;
	ULxUIManager* UIManager = Subsystem ? Subsystem->GetUIManager() : nullptr;
	ULxInteractionUIManager* InteractionUIManager = UIManager ? UIManager->GetInteractionUIManager() : nullptr;
	if (!InteractionUIManager || !InteractionUIManager->OpenFunctionPage(this, PlayerInteractionComponent))
	{
		return false;
	}

	SetInteractionState(ELxInteractionDataState::Interacting);
	return true;
}
