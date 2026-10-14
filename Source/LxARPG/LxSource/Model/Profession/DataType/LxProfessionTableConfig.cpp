#include "LxProfessionTableConfig.h"

#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"
#include "LxProfessionTreeAsset.h"

namespace
{
	/** 资产由数据管理对象持有，此处只保存查询引用。 */
	TWeakObjectPtr<ULxProfessionTreeAsset> GProfessionTree;
	TMap<FGameplayTag, TSubclassOf<ULxProfessionDefinition>> GProfessionDefinitionClassMap;
}

namespace LxProfessionConfig
{
	void SetProfessionTree(ULxProfessionTreeAsset* Asset) { GProfessionTree = Asset; }
	ULxProfessionTreeAsset* GetProfessionTree() { return GProfessionTree.Get(); }
	void ClearProfessionConfig()
	{
		GProfessionDefinitionClassMap.Empty();
		GProfessionTree.Reset();
	}

	void SetProfessionDefinitionTableRow(const FLxProfessionDefinitionTableRow& InProfessionRow)
	{
		if (!InProfessionRow.ProfessionIDTag.IsValid() || !InProfessionRow.ProfessionClass)
		{
			return;
		}

		GProfessionDefinitionClassMap.Add(InProfessionRow.ProfessionIDTag, InProfessionRow.ProfessionClass);
	}

	const TMap<FGameplayTag, TSubclassOf<ULxProfessionDefinition>>& GetProfessionDefinitionClassMap()
	{
		return GProfessionDefinitionClassMap;
	}

	TSubclassOf<ULxProfessionDefinition> GetProfessionDefinitionClass(FGameplayTag InProfessionIDTag)
	{
		if (const TSubclassOf<ULxProfessionDefinition>* ProfessionClass = GProfessionDefinitionClassMap.Find(InProfessionIDTag))
		{
			return *ProfessionClass;
		}

		return nullptr;
	}
}
