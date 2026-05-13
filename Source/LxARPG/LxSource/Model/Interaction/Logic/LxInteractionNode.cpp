#include "LxInteractionNode.h"

#include "LxInteractionActionComponentBase.h"

void ULxInteractionNode::InitializeInteractionNode(FGameplayTag InInteractionIDTag, FGameplayTag InPromptTextTag,
	ELxInteractionActionType InInteractionType, const TArray<ULxInteractionNode*>& InChildNodes,
	ULxInteractionActionComponentBase* InActionComponent)
{
	InteractionIDTag = InInteractionIDTag;
	PromptTextTag = InPromptTextTag;
	InteractionActionType = InInteractionType;
	ActionComponent = InActionComponent;
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

bool ULxInteractionNode::IsNodeValid() const
{
	return ValidateActionComponentType();
}

bool ULxInteractionNode::ValidateActionComponentType() const
{
	if (!ActionComponent)
	{
		// 对话和入口节点可以不绑定具体功能组件。
		return true;
	}

	return ActionComponent->GetInteractionActionType() == InteractionActionType && ActionComponent->IsInteractionValid();
}
