#include "LxInteractionNodeFunctionLibrary.h"

#include "LxInteractionNode.h"

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionNode(UObject* Outer, FGameplayTag PromptTextTag,
	ELxInteractionActionType InteractionType, const TArray<ULxInteractionNode*>& ChildNodes,
	FLxInteractionRequirement Requirement)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptTextTag, InteractionType, ChildNodes, false,
		MoveTemp(Requirement), FLxInteractionFeatureNodeConfig());
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateDialogueInteractionNode(UObject* Outer,
	FGameplayTag PromptTextTag, const TArray<ULxInteractionNode*>& ChildNodes,
	FGameplayTag NpcDialogueTextTag, FLxInteractionRequirement Requirement)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptTextTag, ELxInteractionActionType::Dialogue, ChildNodes, false,
		MoveTemp(Requirement), FLxInteractionFeatureNodeConfig(), NpcDialogueTextTag);
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateEntranceInteractionNode(UObject* Outer,
	FGameplayTag PromptTextTag, const TArray<ULxInteractionNode*>& ChildNodes,
	FLxInteractionRequirement Requirement)
{
	return CreateInteractionNode(Outer, PromptTextTag, ELxInteractionActionType::Entrance, ChildNodes,
		MoveTemp(Requirement));
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionExitNode(UObject* Outer,
	FGameplayTag PromptTextTag)
{
	return CreateInteractionNode(Outer, PromptTextTag, ELxInteractionActionType::InteractionExit,
		TArray<ULxInteractionNode*>(), FLxInteractionRequirement());
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateFunctionInteractionNode(UObject* Outer,
	FGameplayTag PromptTextTag, ELxInteractionActionType InteractionType,
	FLxInteractionFeatureNodeConfig FeatureConfig, FLxInteractionRequirement Requirement)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptTextTag, InteractionType, TArray<ULxInteractionNode*>(), true,
		MoveTemp(Requirement), MoveTemp(FeatureConfig));
	return NewNode;
}
