#include "LxInteractionActionComponentBase.h"

#include "GameFramework/Actor.h"
#include "LxInteractableComponent.h"
#include "LxInteractionNode.h"
#include "Net/UnrealNetwork.h"

void ULxInteractionActionComponentBase::InitializeInteractionFeature(
	ULxInteractableComponent* InOwnerComponent, ULxInteractionNode* InOwnerNode, int32 InRuntimeNodeIndex)
{
	OwnerInteractableComponent = InOwnerComponent;
	OwnerInteractionNode = InOwnerNode;
	RuntimeNodeIndex = InRuntimeNodeIndex;
	if (OwnerInteractionNode)
	{
		PromptTextTag = OwnerInteractionNode->GetConfiguredPromptTextTag();
		Requirement = OwnerInteractionNode->GetInteractionRequirement();
	}

	OnInitializeInteractionFeature();
}

void ULxInteractionActionComponentBase::ShutdownInteractionFeature()
{
	OnShutdownInteractionFeature();
	OwnerInteractionNode = nullptr;
	OwnerInteractableComponent = nullptr;
}

ULxInteractableComponent* ULxInteractionActionComponentBase::GetInteractableComponent() const
{
	return OwnerInteractableComponent ? OwnerInteractableComponent.Get() : Cast<ULxInteractableComponent>(GetOuter());
}

AActor* ULxInteractionActionComponentBase::GetOwner() const
{
	const ULxInteractableComponent* InteractableComponent = GetInteractableComponent();
	return InteractableComponent ? InteractableComponent->GetOwner() : nullptr;
}

UWorld* ULxInteractionActionComponentBase::GetWorld() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->GetWorld() : nullptr;
}

void ULxInteractionActionComponentBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxInteractionActionComponentBase, InteractionActionType);
	DOREPLIFETIME(ULxInteractionActionComponentBase, PromptTextTag);
	DOREPLIFETIME(ULxInteractionActionComponentBase, InteractionState);
	DOREPLIFETIME(ULxInteractionActionComponentBase, bOpenFunctionUI);
	DOREPLIFETIME(ULxInteractionActionComponentBase, RuntimeNodeIndex);
}

FGameplayTag ULxInteractionActionComponentBase::GetPromptTextTag() const
{
	return PromptTextTag;
}

void ULxInteractionActionComponentBase::SetPromptTextTag(FGameplayTag InPromptTextTag)
{
	PromptTextTag = InPromptTextTag;
	NotifyFeatureDataChanged();
}

void ULxInteractionActionComponentBase::SetInteractionState(ELxInteractionDataState InState)
{
	if (InteractionState == InState)
	{
		return;
	}

	InteractionState = InState;
	OnInteractionStateChanged.Broadcast(InteractionState);
	NotifyFeatureDataChanged();
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

void ULxInteractionActionComponentBase::OnInitializeInteractionFeature_Implementation()
{
}

void ULxInteractionActionComponentBase::OnShutdownInteractionFeature_Implementation()
{
}

void ULxInteractionActionComponentBase::NotifyFeatureDataChanged()
{
	OnDataChange.Broadcast();
	if (ULxInteractableComponent* InteractableComponent = GetInteractableComponent())
	{
		InteractableComponent->RefreshInteractionOptions();
	}
}

void ULxInteractionActionComponentBase::OnRep_InteractionState()
{
	OnInteractionStateChanged.Broadcast(InteractionState);
	NotifyFeatureDataChanged();
}
