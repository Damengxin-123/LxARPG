#include "LxInteractionTreeAssetFactory.h"
#include "LxInteractionTreeEdGraph.h"

ULxInteractionTreeAssetFactory::ULxInteractionTreeAssetFactory()
{
	SupportedClass = ULxInteractionTreeAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* ULxInteractionTreeAssetFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>(Parent, Class, Name, Flags | RF_Transactional);
	ULxInteractionTreeEdGraph* Graph = NewObject<ULxInteractionTreeEdGraph>(Asset, TEXT("交互树图"), RF_Transactional);
	Graph->Schema = ULxInteractionTreeEdGraphSchema::StaticClass();
	Asset->EditorGraph = Graph;
	Graph->EnsureStartNode();
	return Asset;
}

