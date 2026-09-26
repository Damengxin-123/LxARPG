#include "LxInteractionActionComponentBase.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveData.h"

#include "GameFramework/Actor.h"
#include "LxInteractableComponent.h"
#include "LxInteractionNode.h"
#include "Net/UnrealNetwork.h"

bool ULxInteractionActionComponentBase::CapturePersistentData(FLxInteractionFeatureSaveRecord& OutRecord) const
{
	if (!OwnerInteractionNode || !OwnerInteractionNode->GetPersistentNodeID().IsValid()) return false;
	OutRecord = FLxInteractionFeatureSaveRecord();
	OutRecord.NodeID = OwnerInteractionNode->GetPersistentNodeID();
	OutRecord.InteractionType = InteractionActionType;
	OutRecord.InteractionState = InteractionState == ELxInteractionDataState::Interacting
		|| InteractionState == ELxInteractionDataState::Occupied
		? ELxInteractionDataState::Interactable : InteractionState;
	return true;
}

bool ULxInteractionActionComponentBase::CanRestorePersistentData(const FLxInteractionFeatureSaveRecord& InRecord) const
{
	return (!GetOwner() || GetOwner()->HasAuthority()) && OwnerInteractionNode
		&& InRecord.NodeID.IsValid() && OwnerInteractionNode->GetPersistentNodeID() == InRecord.NodeID
		&& InteractionActionType == InRecord.InteractionType
		&& StaticEnum<ELxInteractionDataState>()->IsValidEnumValue(static_cast<int64>(InRecord.InteractionState));
}

bool ULxInteractionActionComponentBase::RestorePersistentData(const FLxInteractionFeatureSaveRecord& InRecord)
{
	if (!CanRestorePersistentData(InRecord)) return false;
	SetInteractionState(InRecord.InteractionState == ELxInteractionDataState::Interacting
		|| InRecord.InteractionState == ELxInteractionDataState::Occupied
		? ELxInteractionDataState::Interactable : InRecord.InteractionState);
	if (GetOwner()) GetOwner()->ForceNetUpdate();
	return true;
}

void ULxInteractionActionComponentBase::InitializeInteractionFeature(
	ULxInteractableComponent* InOwnerComponent, ULxInteractionNode* InOwnerNode, int32 InRuntimeNodeIndex)
{
	OwnerInteractableComponent = InOwnerComponent;
	OwnerInteractionNode = InOwnerNode;
	RuntimeNodeIndex = InRuntimeNodeIndex;
	if (!GetOwner() || GetOwner()->HasAuthority())
		InteractionTreeRevision = InOwnerComponent ? InOwnerComponent->GetInteractionTreeRevision() : INDEX_NONE;
	if (OwnerInteractionNode)
	{
		if (!GetOwner() || GetOwner()->HasAuthority())
			PromptText = OwnerInteractionNode->GetConfiguredPromptText();
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
	DOREPLIFETIME(ULxInteractionActionComponentBase, PromptText);
	DOREPLIFETIME(ULxInteractionActionComponentBase, InteractionState);
	DOREPLIFETIME(ULxInteractionActionComponentBase, bOpenFunctionUI);
	DOREPLIFETIME(ULxInteractionActionComponentBase, RuntimeNodeIndex);
	DOREPLIFETIME(ULxInteractionActionComponentBase, InteractionTreeRevision);
}

FText ULxInteractionActionComponentBase::GetPromptText() const
{
	return PromptText;
}

void ULxInteractionActionComponentBase::SetPromptText(FText InPromptText)
{
	PromptText = InPromptText;
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

void ULxInteractionActionComponentBase::OnRep_RuntimeBinding()
{
	if (ULxInteractableComponent* Component = GetInteractableComponent())
		Component->RefreshReplicatedInteractionFeatures();
}
