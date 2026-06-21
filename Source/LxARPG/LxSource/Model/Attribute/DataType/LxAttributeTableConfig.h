#pragma once

#include "CoreMinimal.h"
#include "LxAttributeData.h"

namespace LxAttributeConfig
{
	/** 清空已加载的角色属性配置缓存。 */
	void ClearAttributeConfig();

	/** 写入完整的角色属性定义表。 */
	void SetAttributeDataMap(const TMap<FGameplayTag, FLxAttributeData>& InAttributeDataMap);

	/** 写入完整的种族基础属性值表。 */
	void SetCharacterRaceBaseAttributeValueMap(const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& InRaceBaseValueMap);

	/** 写入单个属性定义。 */
	void SetAttributeDataConfig(const FLxAttributeData& InAttributeData);

	/** 写入单个种族的基础属性值列表。 */
	void SetCharacterRaceBaseAttributeValues(ELxCharacterRaceType InRaceType, const TArray<FLxAttributeValueConfig>& InBaseValueList);

	/** 获取完整的角色属性定义表。 */
	const TMap<FGameplayTag, FLxAttributeData>& GetAttributeDataMap();

	/** 获取完整的种族基础属性值表。 */
	const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& GetCharacterRaceBaseAttributeValueMap();

	/** 从已加载的属性定义表中获取单个属性配置。 */
	const FLxAttributeData* GetAttributeDataConfig(FGameplayTag InAttributeIDTag);

	/** 按角色种族获取运行时属性表。 */
	TMap<FGameplayTag, FLxAttributeData> GetCharacterAttributeDataByRaceType(ELxCharacterRaceType InRaceType);
}

namespace LxAttributeTools
{
	/** 解析属性数据的标签 ID。 */
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeData& InAttributeData);

	/** 解析属性数值覆盖配置的标签 ID。 */
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeValueConfig& InValueConfig);

	/** 解析属性衍生规则的目标属性标签 ID。 */
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeDerivedRule& InDerivedRule);

	/** 检查属性数据中的标签 ID 是否有效。 */
	bool NormalizeAttributeIDTag(FLxAttributeData& InOutAttributeData);

	/** 获取属性的显示文本。 */
	FText GetAttributeDisplayText(const FLxAttributeData& AttributeData);
}
