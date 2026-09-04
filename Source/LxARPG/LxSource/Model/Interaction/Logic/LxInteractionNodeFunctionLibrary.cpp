#include "LxInteractionNodeFunctionLibrary.h"

#include "LxInteractionActionComponentBase.h"
#include "LxInteractionNode.h"

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
	ELxInteractionActionType InteractionType, const TArray<ULxInteractionNode*>& ChildNodes)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptTextTag, InteractionType, ChildNodes);
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateDialogueInteractionNode(UObject* Outer,
	FGameplayTag PromptTextTag, const TArray<ULxInteractionNode*>& ChildNodes,
	FGameplayTag NpcDialogueTextTag)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptTextTag, ELxInteractionActionType::Dialogue, ChildNodes,
		nullptr, NpcDialogueTextTag);
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateEntranceInteractionNode(UObject* Outer,
	FGameplayTag PromptTextTag, const TArray<ULxInteractionNode*>& ChildNodes)
{
	return CreateInteractionNode(Outer, PromptTextTag, ELxInteractionActionType::Entrance, ChildNodes);
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionExitNode(UObject* Outer,
	FGameplayTag PromptTextTag)
{
	return CreateInteractionNode(Outer, PromptTextTag, ELxInteractionActionType::InteractionExit,
		TArray<ULxInteractionNode*>());
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateFunctionInteractionNode(UObject* Outer,
	FGameplayTag PromptTextTag, ELxInteractionActionType InteractionType,
	ULxInteractionActionComponentBase* ActionComponent)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	if (ActionComponent)
	{
		ActionComponent->SetPromptTextTag(PromptTextTag);
	}
	NewNode->InitializeInteractionNode(PromptTextTag, InteractionType, TArray<ULxInteractionNode*>(), ActionComponent);
	return NewNode;
}
