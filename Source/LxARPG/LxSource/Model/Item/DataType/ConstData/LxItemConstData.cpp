#include "LxItemConstData.h"

namespace
{
	TMap<FGameplayTag, FLxEquipmentInformation> GEquipmentItemMap;
	TMap<FGameplayTag, FLxConsumableInformation> GConsumableItemMap;
	TMap<FGameplayTag, FLxMaterialInformation> GMaterialItemMap;
	TMap<FGameplayTag, FLxBuffInformation> GBuffItemMap;

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
	}

	void SetEquipmentItemData(const FLxEquipmentInformation& InItemData)
	{
		if (IsValidItemConfig(InItemData))
		{
			GEquipmentItemMap.Add(InItemData.ItemIDTag, InItemData);
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
			GBuffItemMap.Add(InItemData.ItemIDTag, InItemData);
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

		return nullptr;
	}
}
