#include "LxTriggerMechanismInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxInteractableComponent.h"
#include "Net/UnrealNetwork.h"

ULxTriggerMechanismInteractionComponent::ULxTriggerMechanismInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::TriggerMechanism;
	bOpenFunctionUI = false;
}

void ULxTriggerMechanismInteractionComponent::ApplyConfig(const FLxTriggerMechanismInteractionConfig& InConfig)
{
	MechanismState = InConfig.InitialState;
	MechanismStatePromptTexts.Reset();
	for (const FLxMechanismStatePromptText& StatePromptText : InConfig.StatePromptTexts)
	{
		MechanismStatePromptTexts.Add(StatePromptText.MechanismState, StatePromptText.PromptText);
	}
}

void ULxTriggerMechanismInteractionComponent::OnInitializeInteractionFeature_Implementation()
{
	Super::OnInitializeInteractionFeature_Implementation();
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicates(true);
	}
}

void ULxTriggerMechanismInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxTriggerMechanismInteractionComponent, MechanismState);
}

bool ULxTriggerMechanismInteractionComponent::TriggerMechanism_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		const ALxBaseCharacter* OwnerCharacter = PlayerInteractionComponent ? Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner()) : nullptr;
		ALxPlayerController* PlayerController = OwnerCharacter ? Cast<ALxPlayerController>(OwnerCharacter->GetController()) : nullptr;
		if (PlayerController == nullptr)
		{
			return false;
		}

		PlayerController->ServerTriggerMechanism(OwnerActor, GetRuntimeNodeIndex());
		return true;
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
	BroadcastMechanismStateChanged();
}

void ULxTriggerMechanismInteractionComponent::OnRep_MechanismState()
{
	BroadcastMechanismStateChanged();
}

void ULxTriggerMechanismInteractionComponent::BroadcastMechanismStateChanged()
{
	OnMechanismStateChanged.Broadcast(MechanismState);
	if (ULxInteractableComponent* InteractableComponent = GetInteractableComponent())
	{
		InteractableComponent->NotifyMechanismStateChanged(MechanismState);
	}
	NotifyFeatureDataChanged();
}

FText ULxTriggerMechanismInteractionComponent::GetPromptText() const
{
	if (const FText* PromptTextByState = MechanismStatePromptTexts.Find(MechanismState))
	{
		return *PromptTextByState;
	}

	return Super::GetPromptText();
}

bool ULxTriggerMechanismInteractionComponent::ExecuteInteraction_Implementation(
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	return TriggerMechanism(PlayerInteractionComponent);
}
