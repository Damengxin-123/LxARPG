#include "LxTriggerMechanismInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "Net/UnrealNetwork.h"

ULxTriggerMechanismInteractionComponent::ULxTriggerMechanismInteractionComponent()
{
	SetIsReplicatedByDefault(true);
	InteractionActionType = ELxInteractionActionType::TriggerMechanism;
}

void ULxTriggerMechanismInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
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

		PlayerController->ServerTriggerMechanism(OwnerActor);
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
	OnMechanismStateChanged.Broadcast(MechanismState);
	OnDataChange.Broadcast();
}

void ULxTriggerMechanismInteractionComponent::OnRep_MechanismState()
{
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
	ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	return TriggerMechanism(PlayerInteractionComponent);
}
