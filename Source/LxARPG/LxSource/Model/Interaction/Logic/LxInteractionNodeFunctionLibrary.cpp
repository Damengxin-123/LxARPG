#include "LxInteractionNodeFunctionLibrary.h"

#include "LxInteractionNode.h"

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionNode(UObject* Outer, FGameplayTag InteractionIDTag,
	FGameplayTag PromptTextTag, ELxInteractionActionType InteractionType, const TArray<ULxInteractionNode*>& ChildNodes)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(InteractionIDTag, PromptTextTag, InteractionType, ChildNodes);
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateDialogueInteractionNode(UObject* Outer,
	FGameplayTag InteractionIDTag, FGameplayTag PromptTextTag, const TArray<ULxInteractionNode*>& ChildNodes)
{
	return CreateInteractionNode(Outer, InteractionIDTag, PromptTextTag, ELxInteractionActionType::Dialogue, ChildNodes);
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateFunctionInteractionNode(UObject* Outer,
	FGameplayTag InteractionIDTag, FGameplayTag PromptTextTag, ELxInteractionActionType InteractionType,
	ULxInteractionActionComponentBase* ActionComponent)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(InteractionIDTag, PromptTextTag, InteractionType, TArray<ULxInteractionNode*>(), ActionComponent);
	return NewNode;
}
