#include "LxInteractableComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Interaction/Interface/LxInteractionReceiverInterface.h"
#include "LxInteractionNode.h"

void ULxInteractableComponent::SetRootInteractionNodes(const TArray<ULxInteractionNode*>& InRootNodes)
{
	RootInteractionNodes.Reset();
	for (ULxInteractionNode* RootNode : InRootNodes)
	{
		AddRootInteractionNode(RootNode);
	}
	RefreshInteractionOptions();
}

void ULxInteractableComponent::AddRootInteractionNode(ULxInteractionNode* InRootNode)
{
	if (!InRootNode || RootInteractionNodes.Contains(InRootNode))
	{
		return;
	}

	InRootNode->SetParentNode(nullptr);
	RootInteractionNodes.Add(InRootNode);
	RefreshInteractionOptions();
}

TArray<ULxInteractionNode*> ULxInteractableComponent::GetRootInteractionNodes() const
{
	TArray<ULxInteractionNode*> Result;
	for (ULxInteractionNode* RootNode : RootInteractionNodes)
	{
		Result.Add(RootNode);
	}
	return Result;
}

TArray<ULxInteractionNode*> ULxInteractableComponent::GetValidRootInteractionNodes() const
{
	TArray<ULxInteractionNode*> Result;
	for (ULxInteractionNode* RootNode : RootInteractionNodes)
	{
		if (RootNode && RootNode->IsNodeValid())
		{
			Result.Add(RootNode);
		}
	}
	return Result;
}

bool ULxInteractableComponent::HasValidInteraction() const
{
	return GetValidRootInteractionNodes().Num() > 0;
}

void ULxInteractableComponent::RefreshInteractionOptions()
{
	OnInteractableOptionsChanged.Broadcast();
	OnDataChange.Broadcast();
}

void ULxInteractableComponent::HandleInteractionRangeBeginOverlap(AActor* OtherActor)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(ULxInteractionReceiverInterface::StaticClass()))
	{
		return;
	}

	ILxInteractionReceiverInterface::Execute_ReceiveInteractableComponent(OtherActor, this);
}

void ULxInteractableComponent::HandleInteractionRangeEndOverlap(AActor* OtherActor)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(ULxInteractionReceiverInterface::StaticClass()))
	{
		return;
	}

	ILxInteractionReceiverInterface::Execute_RemoveInteractableComponent(OtherActor, this);
}
