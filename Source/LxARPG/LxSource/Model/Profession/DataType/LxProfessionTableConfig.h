#pragma once

#include "CoreMinimal.h"
#include "LxProfessionTypes.h"

class ULxProfessionTreeAsset;

namespace LxProfessionConfig
{
	/** 设置角色运行时使用的唯一职业体系资产。 */
	LXARPG_API void SetProfessionTree(ULxProfessionTreeAsset* Asset);
	/** 获取当前职业体系，未配置时返回空而不回退旧职业表。 */
	LXARPG_API ULxProfessionTreeAsset* GetProfessionTree();
	/** 清空已加载的职业定义表缓存。 */
	void ClearProfessionConfig();

	/** 写入单条职业定义表数据。 */
	void SetProfessionDefinitionTableRow(const FLxProfessionDefinitionTableRow& InProfessionRow);

	/** 获取完整的职业定义类型映射表。 */
	const TMap<FGameplayTag, TSubclassOf<ULxProfessionDefinition>>& GetProfessionDefinitionClassMap();

	/** 按职业标签 ID 查询职业定义类型。 */
	TSubclassOf<ULxProfessionDefinition> GetProfessionDefinitionClass(FGameplayTag InProfessionIDTag);
}
