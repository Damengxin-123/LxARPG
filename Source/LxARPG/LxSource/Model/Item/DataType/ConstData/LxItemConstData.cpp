#include "LxItemConstData.h"

namespace
{
	TMap<FGameplayTag, FLxEquipmentInformation> GEquipmentItemMap;
	TMap<FGameplayTag, FLxConsumableInformation> GConsumableItemMap;
	TMap<FGameplayTag, FLxMaterialInformation> GMaterialItemMap;
	TMap<FGameplayTag, FLxBuffInformation> GBuffItemMap;
	TMap<FGameplayTag, FLxSkillItemInformation> GSkillItemMap;

	template<typename ItemDataType>
	bool IsValidItemConfig(const ItemDataType& InItemData)
	{
		return InItemData.ItemIDTag.IsValid();
	}
}

namespace LxItemConfig
{
	void ClearItemConfig()
	{
		GEquipmentItemMap.Empty();
		GConsumableItemMap.Empty();
		GMaterialItemMap.Empty();
		GBuffItemMap.Empty();
		GSkillItemMap.Empty();
	}

	void SetEquipmentItemData(const FLxEquipmentInformation& InItemData)
	{
		if (IsValidItemConfig(InItemData))
		{
			// 装备不可堆叠；归一化旧数据表中可能已经保存的数量配置。
			FLxEquipmentInformation NormalizedItemData = InItemData;
			NormalizedItemData.ItemCount = 1;
			NormalizedItemData.ItemCountMax = 1;
			GEquipmentItemMap.Add(InItemData.ItemIDTag, MoveTemp(NormalizedItemData));
		}
	}

	void SetConsumableItemData(const FLxConsumableInformation& InItemData)
	{
		if (IsValidItemConfig(InItemData))
		{
			GConsumableItemMap.Add(InItemData.ItemIDTag, InItemData);
		}
	}

	void SetMaterialItemData(const FLxMaterialInformation& InItemData)
	{
		if (IsValidItemConfig(InItemData))
		{
			GMaterialItemMap.Add(InItemData.ItemIDTag, InItemData);
		}
	}

	void SetBuffItemData(const FLxBuffInformation& InItemData)
	{
		if (IsValidItemConfig(InItemData))
		{
			// Buff 不可堆叠；归一化旧数据表中可能已经保存的数量配置。
			FLxBuffInformation NormalizedItemData = InItemData;
			NormalizedItemData.ItemCount = 1;
			NormalizedItemData.ItemCountMax = 1;
			GBuffItemMap.Add(InItemData.ItemIDTag, MoveTemp(NormalizedItemData));
		}
	}

	void SetSkillItemData(const FLxSkillItemInformation& InItemData)
	{
		if (IsValidItemConfig(InItemData))
		{
			// 技能物品不可堆叠；归一化旧数据表中可能已经保存的数量配置。
			FLxSkillItemInformation NormalizedItemData = InItemData;
			NormalizedItemData.ItemCount = 1;
			NormalizedItemData.ItemCountMax = 1;
			GSkillItemMap.Add(InItemData.ItemIDTag, MoveTemp(NormalizedItemData));
		}
	}

	const TMap<FGameplayTag, FLxEquipmentInformation>& GetEquipmentItemMap()
	{
		return GEquipmentItemMap;
	}

	const TMap<FGameplayTag, FLxConsumableInformation>& GetConsumableItemMap()
	{
		return GConsumableItemMap;
	}

	const TMap<FGameplayTag, FLxMaterialInformation>& GetMaterialItemMap()
	{
		return GMaterialItemMap;
	}

	const TMap<FGameplayTag, FLxBuffInformation>& GetBuffItemMap()
	{
		return GBuffItemMap;
	}

	const TMap<FGameplayTag, FLxSkillItemInformation>& GetSkillItemMap()
	{
		return GSkillItemMap;
	}

	const FLxItemInformationBase* GetItemData(FGameplayTag InItemIDTag)
	{
		if (!InItemIDTag.IsValid())
		{
			return nullptr;
		}

		if (const FLxEquipmentInformation* EquipmentData = GEquipmentItemMap.Find(InItemIDTag))
		{
			return EquipmentData;
		}
		if (const FLxConsumableInformation* ConsumableData = GConsumableItemMap.Find(InItemIDTag))
		{
			return ConsumableData;
		}
		if (const FLxMaterialInformation* MaterialData = GMaterialItemMap.Find(InItemIDTag))
		{
			return MaterialData;
		}
		if (const FLxBuffInformation* BuffData = GBuffItemMap.Find(InItemIDTag))
		{
			return BuffData;
		}
		if (const FLxSkillItemInformation* SkillData = GSkillItemMap.Find(InItemIDTag))
		{
			return SkillData;
		}

		return nullptr;
	}
}
