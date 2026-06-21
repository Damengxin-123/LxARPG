#pragma once

#include "CoreMinimal.h"
#include "LxProfessionTypes.h"

namespace LxProfessionConfig
{
	/** 清空已加载的职业定义表缓存。 */
	void ClearProfessionConfig();

	/** 写入单条职业定义表数据。 */
	void SetProfessionDefinitionTableRow(const FLxProfessionDefinitionTableRow& InProfessionRow);

	/** 获取完整的职业定义类型映射表。 */
	const TMap<FGameplayTag, TSubclassOf<ULxProfessionDefinition>>& GetProfessionDefinitionClassMap();

	/** 按职业标签 ID 查询职业定义类型。 */
	TSubclassOf<ULxProfessionDefinition> GetProfessionDefinitionClass(FGameplayTag InProfessionIDTag);
}
