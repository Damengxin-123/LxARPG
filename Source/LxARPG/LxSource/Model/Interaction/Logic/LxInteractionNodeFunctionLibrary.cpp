#include "LxInteractionNodeFunctionLibrary.h"

#include "LxInteractionNode.h"

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionNode(UObject* Outer, FText PromptText,
	FText NpcDialogueText, ELxInteractionActionType InteractionType,
	FLxInteractionRequirement Requirement, TArray<ULxInteractionNode*> ChildNodes)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptText, InteractionType, ChildNodes,
		MoveTemp(Requirement), NpcDialogueText);
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateInteractionExitNode(UObject* Outer,
	FText PromptText)
{
	return CreateInteractionNode(Outer, PromptText, FText(), ELxInteractionActionType::InteractionExit,
		FLxInteractionRequirement(), TArray<ULxInteractionNode*>());
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateFunctionInteractionNode(UObject* Outer,
	FText PromptText, ELxInteractionActionType InteractionType,
	FLxInteractionRequirement Requirement, TArray<ULxInteractionNode*> ChildNodes)
{
	UObject* NodeOuter = Outer ? Outer : GetTransientPackage();
	ULxInteractionNode* NewNode = NewObject<ULxInteractionNode>(NodeOuter);
	NewNode->InitializeInteractionNode(PromptText, InteractionType, ChildNodes,
		MoveTemp(Requirement));
	return NewNode;
}

ULxInteractionNode* ULxInteractionNodeFunctionLibrary::CreateQuestInteractionNode(UObject* Outer,
	FText PromptText, FGameplayTag QuestSeriesId, FGameplayTag QuestId,
	FLxInteractionRequirement Requirement, TArray<ULxInteractionNode*> ChildNodes, bool bUseQuestDisplayText)
{
	ULxInteractionNode* NewNode = CreateFunctionInteractionNode(Outer, PromptText,
		ELxInteractionActionType::Quest, MoveTemp(Requirement), MoveTemp(ChildNodes));
	FLxQuestInteractionConfig QuestConfig;
	QuestConfig.bUseQuestDisplayText = bUseQuestDisplayText;
	QuestConfig.QuestSeriesId = QuestSeriesId;
	QuestConfig.QuestId = QuestId;
	NewNode->SetQuestInteractionConfig(QuestConfig);
	return NewNode;
}
