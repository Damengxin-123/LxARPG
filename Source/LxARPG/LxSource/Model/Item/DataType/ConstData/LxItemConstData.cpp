#include "LxItemConstData.h"

namespace
{
	TMap<FLxItemID, FLxEquipmentInformation> GEquipmentItemMap;
	TMap<FLxItemID, FLxConsumableInformation> GConsumableItemMap;
	TMap<FLxItemID, FLxMaterialInformation> GMaterialItemMap;
	TMap<FLxItemID, FLxBuffInformation> GBuffItemMap;
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
		if (InItemData.ItemID != ItemIDNone)
		{
			GEquipmentItemMap.Add(InItemData.ItemID, InItemData);
		}
	}

	void SetConsumableItemData(const FLxConsumableInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			GConsumableItemMap.Add(InItemData.ItemID, InItemData);
		}
	}

	void SetMaterialItemData(const FLxMaterialInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			GMaterialItemMap.Add(InItemData.ItemID, InItemData);
		}
	}

	void SetBuffItemData(const FLxBuffInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			GBuffItemMap.Add(InItemData.ItemID, InItemData);
		}
	}

	const TMap<FLxItemID, FLxEquipmentInformation>& GetEquipmentItemMap()
	{
		return GEquipmentItemMap;
	}

	const TMap<FLxItemID, FLxConsumableInformation>& GetConsumableItemMap()
	{
		return GConsumableItemMap;
	}

	const TMap<FLxItemID, FLxMaterialInformation>& GetMaterialItemMap()
	{
		return GMaterialItemMap;
	}

	const TMap<FLxItemID, FLxBuffInformation>& GetBuffItemMap()
	{
		return GBuffItemMap;
	}

	const FLxItemInformationBase* GetItemData(ELxItemType InItemType, FLxItemID InItemID)
	{
		switch (InItemType)
		{
		case ELxItemType::Equipment:
			return GEquipmentItemMap.Find(InItemID);
		case ELxItemType::Consumable:
			return GConsumableItemMap.Find(InItemID);
		case ELxItemType::Material:
			return GMaterialItemMap.Find(InItemID);
		case ELxItemType::Buff:
			return GBuffItemMap.Find(InItemID);
		default:
			return nullptr;
		}
	}
}
