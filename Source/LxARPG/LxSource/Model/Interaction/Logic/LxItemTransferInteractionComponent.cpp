#include "LxItemTransferInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"

ULxItemTransferInteractionComponent::ULxItemTransferInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::ItemTransfer;
	bOpenFunctionUI = false;
}

void ULxItemTransferInteractionComponent::ApplyConfig(const FLxItemTransferInteractionConfig& InConfig)
{
	ItemTransferList = InConfig.ItemList;
	ItemTransferDirection = InConfig.Direction;
}

bool ULxItemTransferInteractionComponent::CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const
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

bool ULxItemTransferInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner());
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && !OwnerActor->HasAuthority())
	{
		ALxPlayerController* PlayerController = OwnerCharacter ? Cast<ALxPlayerController>(OwnerCharacter->GetController()) : nullptr;
		if (PlayerController == nullptr)
		{
			return false;
		}

		PlayerController->ServerExecuteItemTransfer(OwnerActor, GetRuntimeNodeIndex());
		return true;
	}

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
