#include "LxInteractionNode.h"

#include "LxInteractionActionComponentBase.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

void ULxInteractionNode::InitializeInteractionNode(FGameplayTag InPromptTextTag,
	ELxInteractionActionType InInteractionType, const TArray<ULxInteractionNode*>& InChildNodes,
	bool bInIsFunctionNode, FLxInteractionRequirement InRequirement,
	FLxInteractionFeatureNodeConfig InFeatureConfig, FGameplayTag InNpcDialogueTextTag)
{
	PromptTextTag = InPromptTextTag;
	InteractionActionType = InInteractionType;
	bIsFunctionNode = bInIsFunctionNode;
	Requirement = MoveTemp(InRequirement);
	FeatureConfig = MoveTemp(InFeatureConfig);
	ActionComponent = nullptr;
	NpcDialogueTextTag = InNpcDialogueTextTag;
	RuntimeNodeIndex = INDEX_NONE;
	ChildNodes.Reset();
	AddChildNodes(InChildNodes);
}

void ULxInteractionNode::AddChildNode(ULxInteractionNode* InChildNode)
{
	if (!InChildNode || ChildNodes.Contains(InChildNode))
	{
		return;
	}

	InChildNode->SetParentNode(this);
	ChildNodes.Add(InChildNode);
}

void ULxInteractionNode::AddChildNodes(const TArray<ULxInteractionNode*>& InChildNodes)
{
	for (ULxInteractionNode* ChildNode : InChildNodes)
	{
		AddChildNode(ChildNode);
	}
}

TArray<ULxInteractionNode*> ULxInteractionNode::GetChildNodes() const
{
	TArray<ULxInteractionNode*> Result;
	for (ULxInteractionNode* ChildNode : ChildNodes)
	{
		Result.Add(ChildNode);
	}
	return Result;
}

TArray<ULxInteractionNode*> ULxInteractionNode::GetValidChildNodes() const
{
	TArray<ULxInteractionNode*> ValidChildNodes;
	for (ULxInteractionNode* ChildNode : ChildNodes)
	{
		if (ChildNode && ChildNode->IsNodeValid())
		{
			ValidChildNodes.Add(ChildNode);
		}
	}
	return ValidChildNodes;
}

FGameplayTag ULxInteractionNode::GetPromptTextTag() const
{
	return ActionComponent ? ActionComponent->GetPromptTextTag() : PromptTextTag;
}

bool ULxInteractionNode::IsNodeValid() const
{
	return ValidateNodeType() && (!bIsFunctionNode || ValidateActionComponentType());
}

bool ULxInteractionNode::IsNodeInteractable(ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	if (!IsNodeValid() || !CheckCommonRequirement(PlayerInteractionComponent))
	{
		return false;
	}

	return !bIsFunctionNode || (ActionComponent && ActionComponent->CheckInteractionRequirement(PlayerInteractionComponent));
}

bool ULxInteractionNode::CheckCommonRequirement(ULxPlayerInteractionModule* PlayerInteractionComponent) const
{
	const ALxPlayerCharacter* PlayerCharacter = PlayerInteractionComponent
		? PlayerInteractionComponent->GetPlayerCharacter()
		: nullptr;
	const ULxCharacterDataTransferComponent* DataTransferComponent = PlayerCharacter
		? PlayerCharacter->GetCharacterDataTransferComponent()
		: nullptr;
	if (!DataTransferComponent)
	{
		return false;
	}

	if (!Requirement.RequiredItems.IsEmpty()
		&& !DataTransferComponent->CheckHaveBackpackItemList(Requirement.RequiredItems))
	{
		return false;
	}

	for (const FLxInteractionAttributeRequirement& AttributeRequirement : Requirement.RequiredAttributes)
	{
		float AttributeValue = 0.0f;
		if (!DataTransferComponent->QueryCharacterAttributeValue(AttributeRequirement.AttributeIDTag, AttributeValue)
			|| AttributeValue < AttributeRequirement.MinValue)
		{
			return false;
		}
	}

	if (!Requirement.RequiredStateTags.IsEmpty())
	{
		FGameplayTagContainer CurrentStateTags;
		DataTransferComponent->GetAllCharacterStateTags(CurrentStateTags);
		if (!CurrentStateTags.HasAll(Requirement.RequiredStateTags))
		{
			return false;
		}
	}

	return true;
}

bool ULxInteractionNode::ValidateActionComponentType() const
{
	if (!ActionComponent)
	{
		return false;
	}

	return ActionComponent->GetInteractionActionType() == InteractionActionType && ActionComponent->IsInteractionValid();
}

bool ULxInteractionNode::ValidateNodeType() const
{
	const bool bOrdinaryInteractionType = InteractionActionType == ELxInteractionActionType::Entrance
		|| InteractionActionType == ELxInteractionActionType::Dialogue
		|| InteractionActionType == ELxInteractionActionType::InteractionExit;
	return bIsFunctionNode ? !bOrdinaryInteractionType : bOrdinaryInteractionType;
}
