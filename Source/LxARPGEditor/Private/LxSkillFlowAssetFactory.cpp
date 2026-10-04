#include "LxSkillFlowAssetFactory.h"
#include "LxSkillFlowEdGraph.h"

ULxSkillFlowAssetFactory::ULxSkillFlowAssetFactory()
{
	SupportedClass = ULxSkillFlowAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* ULxSkillFlowAssetFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULxSkillFlowAsset* Asset = NewObject<ULxSkillFlowAsset>(Parent, Class, Name, Flags | RF_Transactional);
	ULxSkillFlowEdGraph* Graph = NewObject<ULxSkillFlowEdGraph>(Asset, TEXT("技能流程图"), RF_Transactional);
	Graph->Schema = ULxSkillFlowEdGraphSchema::StaticClass();
	Asset->EditorGraph = Graph;
	Graph->EnsureEntry();
	return Asset;
}
