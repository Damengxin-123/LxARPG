#include "LxAIBehaviorTreeAssetFactory.h"

#include "LxAIBehaviorTreeEdGraph.h"

ULxAIBehaviorTreeAssetFactory::ULxAIBehaviorTreeAssetFactory()
{
	SupportedClass = ULxAIBehaviorTreeAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* ULxAIBehaviorTreeAssetFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULxAIBehaviorTreeAsset* Asset = NewObject<ULxAIBehaviorTreeAsset>(Parent, Class, Name, Flags | RF_Transactional);
	InitializeDefaultTree(Asset);
	return Asset;
}

void ULxAIBehaviorTreeAssetFactory::InitializeDefaultTree(ULxAIBehaviorTreeAsset* Asset)
{
	if (!Asset) return;
	for (const ULxAIBehaviorTreeNodeData* Data : Asset->Nodes)
		if (!Data || Data->Kind != ELxAIBehaviorNodeKind::Entry) return;
	ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(Asset->EditorGraph);
	if (!Graph)
	{
		if (Asset->EditorGraph) return;
		Graph = NewObject<ULxAIBehaviorTreeEdGraph>(Asset, TEXT("AI行为树图"), RF_Transactional);
		Graph->Schema = ULxAIBehaviorTreeEdGraphSchema::StaticClass();
		Asset->EditorGraph = Graph;
	}
	if (!Graph->Schema) Graph->Schema = ULxAIBehaviorTreeEdGraphSchema::StaticClass();
	Graph->InitializeDefaultTree();
}
