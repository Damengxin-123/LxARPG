#include "LxProfessionTableConfig.h"

#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"

namespace
{
	TMap<FGameplayTag, TSubclassOf<ULxProfessionDefinition>> GProfessionDefinitionClassMap;
}

namespace LxProfessionConfig
{
	void ClearProfessionConfig()
	{
		GProfessionDefinitionClassMap.Empty();
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
