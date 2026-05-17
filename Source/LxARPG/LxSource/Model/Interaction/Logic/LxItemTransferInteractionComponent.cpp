#include "LxItemTransferInteractionComponent.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxPlayerInteractionComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxItemTransferInteractionComponent::ULxItemTransferInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::ItemTransfer;
}

bool ULxItemTransferInteractionComponent::CheckInteractionRequirement_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent) const
{
	if (!Super::CheckInteractionRequirement_Implementation(PlayerInteractionComponent) || ItemTransferList.IsEmpty())
	{
		return false;
	}

	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner());
	const ULxCharacterDataTransferComponent* DataTransferComponent = OwnerCharacter ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (DataTransferComponent == nullptr)
	{
		return false;
	}

	return ItemTransferDirection == ELxItemTransferDirection::AddToPlayer
		? DataTransferComponent->CanAddItemListToBackpack(ItemTransferList)
		: DataTransferComponent->CheckHaveBackpackItemList(ItemTransferList);
}

bool ULxItemTransferInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner());
	ULxCharacterDataTransferComponent* DataTransferComponent = OwnerCharacter ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
	if (DataTransferComponent == nullptr)
	{
		return false;
	}

	const bool bTransferSucceeded = ItemTransferDirection == ELxItemTransferDirection::AddToPlayer
		? DataTransferComponent->AddItemListToBackpack(ItemTransferList)
		: DataTransferComponent->RemoveItemListFromBackpack(ItemTransferList);

	if (bTransferSucceeded)
	{
		OnItemTransferCompleted.Broadcast();
	}
	return bTransferSucceeded;
}
