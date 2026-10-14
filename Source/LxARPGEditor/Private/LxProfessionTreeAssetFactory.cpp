#include "LxProfessionTreeAssetFactory.h"
#include "LxProfessionTreeEdGraph.h"

ULxProfessionTreeAssetFactory::ULxProfessionTreeAssetFactory()
{
	bCreateNew = true; bEditAfterNew = true; SupportedClass = ULxProfessionTreeAsset::StaticClass();
}

UObject* ULxProfessionTreeAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	auto* Asset = NewObject<ULxProfessionTreeAsset>(InParent, Class, Name, Flags | RF_Transactional);
	Asset->EditorGraph = NewObject<ULxProfessionTreeEdGraph>(Asset, TEXT("职业关系图"), RF_Transactional);
	Asset->EditorGraph->Schema = ULxProfessionTreeEdGraphSchema::StaticClass();
	return Asset;
}
