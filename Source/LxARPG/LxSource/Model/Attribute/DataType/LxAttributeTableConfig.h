#pragma once

#include "CoreMinimal.h"
#include "LxAttributeData.h"

namespace LxAttributeConfig
{
	/**
	 * 清空已加载的角色属性配置缓存。
	 *
	 * 通常在重新加载数据表前调用，避免旧数据残留。
	 */
	void ClearAttributeConfig();

	/**
	 * 写入完整的角色属性定义表。
	 *
	 * 该表保存属性规则、标签、衍生规则和显示信息，是所有种族属性组装的基础。
	 */
	void SetAttributeDataMap(const TMap<FGameplayTag, FLxAttributeData>& InAttributeDataMap);

	/**
	 * 写入完整的种族基础属性值表。
	 *
	 * Key 为角色种族，Value 为该种族对属性基础值的覆盖列表。
	 */
	void SetCharacterRaceBaseAttributeValueMap(const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& InRaceBaseValueMap);

	/**
	 * 写入单个属性定义。
	 *
	 * 若属性 ID 已存在，则覆盖旧配置。
	 */
	void SetAttributeDataConfig(const FLxAttributeData& InAttributeData);

	/**
	 * 写入单个种族的基础属性值列表。
	 *
	 * 若种族已存在，则覆盖旧配置。
	 */
	void SetCharacterRaceBaseAttributeValues(ELxCharacterRaceType InRaceType, const TArray<FLxAttributeValueConfig>& InBaseValueList);

	/**
	 * 获取完整的角色属性定义表。
	 */
	const TMap<FGameplayTag, FLxAttributeData>& GetAttributeDataMap();

	/**
	 * 获取完整的种族基础属性值表。
	 */
	const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& GetCharacterRaceBaseAttributeValueMap();

	/**
	 * 从已加载的属性定义表中获取单个属性配置。
	 */
	const FLxAttributeData* GetAttributeDataConfig(FGameplayTag InAttributeIDTag);

	/**
	 * 按角色种族获取运行时属性表。
	 *
	 * 会先复制通用属性定义表，再将该种族配置的基础数值覆盖到对应属性上。
	 */
	TMap<FGameplayTag, FLxAttributeData> GetCharacterAttributeDataByRaceType(ELxCharacterRaceType InRaceType);
}

namespace LxAttributeTools
{
	/**
	 * 将旧版角色属性枚举 ID 转换为角色属性标签 ID。
	 *
	 * 仅用于兼容旧数据；新逻辑应直接配置和传递 AttributeIDTag。
	 */
	FGameplayTag GetAttributeIDTagByLegacyID(ELxCharacterAttributeID InAttributeID);

	/**
	 * 解析属性数据的标签 ID。
	 *
	 * 当 AttributeIDTag 已配置时直接使用标签；否则从旧版枚举 ID 推导标签。
	 */
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeData& InAttributeData);

	/**
	 * 解析属性数值覆盖配置的标签 ID。
	 *
	 * 用于种族或角色模板基础属性覆盖。
	 */
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeValueConfig& InValueConfig);

	/**
	 * 解析属性衍生规则的目标属性标签 ID。
	 */
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeDerivedRule& InDerivedRule);

	/**
	 * 补齐属性数据中的标签 ID。
	 *
	 * @return 成功得到有效属性标签时返回 true。
	 */
	bool NormalizeAttributeIDTag(FLxAttributeData& InOutAttributeData);

	/**
	 * 获取属性的显示文本。
	 *
	 * 该函数根据提供的属性数据生成一个用于UI展示的文本。通常包括属性名称和其当前值，但具体格式依赖于`FLxAttributeData`中的`ShowInfo`字段配置。
	 *
	 * @param AttributeData 属性数据实例，包含要转换为显示文本的属性信息。
	 * @return 返回一个FText对象，表示属性在UI中的显示文本。
	 */
	FText GetAttributeDisplayText(const FLxAttributeData& AttributeData);
}
