#include "LxItemConstData.h"

namespace
{
	TMap<FLxItemID, FLxEquipmentInformation> GEquipmentItemMap;
	TMap<FLxItemID, FLxConsumableInformation> GConsumableItemMap;
	TMap<FLxItemID, FLxMaterialInformation> GMaterialItemMap;
	TMap<FLxItemID, FLxBuffInformation> GBuffItemMap;
	TMap<FGameplayTag, FLxItemQuote> GItemTagIDMap;

	void SetItemTagIDData(const FLxItemInformationBase& InItemData)
	{
		if (InItemData.ItemIDTest.IsValid())
		{
			GItemTagIDMap.Add(InItemData.ItemIDTest, FLxItemQuote(InItemData.ItemType, InItemData.ItemID, 1));
		}
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
		GItemTagIDMap.Empty();
	}

	void SetEquipmentItemData(const FLxEquipmentInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			const FLxEquipmentInformation& StoredItemData = GEquipmentItemMap.Add(InItemData.ItemID, InItemData);
			SetItemTagIDData(StoredItemData);
		}
	}

	void SetConsumableItemData(const FLxConsumableInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			const FLxConsumableInformation& StoredItemData = GConsumableItemMap.Add(InItemData.ItemID, InItemData);
			SetItemTagIDData(StoredItemData);
		}
	}

	void SetMaterialItemData(const FLxMaterialInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			const FLxMaterialInformation& StoredItemData = GMaterialItemMap.Add(InItemData.ItemID, InItemData);
			SetItemTagIDData(StoredItemData);
		}
	}

	void SetBuffItemData(const FLxBuffInformation& InItemData)
	{
		if (InItemData.ItemID != ItemIDNone)
		{
			const FLxBuffInformation& StoredItemData = GBuffItemMap.Add(InItemData.ItemID, InItemData);
			SetItemTagIDData(StoredItemData);
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

	const FLxItemInformationBase* GetItemData(FGameplayTag InItemIDTag)
	{
		if (!InItemIDTag.IsValid())
		{
			return nullptr;
		}

		const FLxItemQuote* ItemQuote = GItemTagIDMap.Find(InItemIDTag);
		return ItemQuote != nullptr ? GetItemData(ItemQuote->ItemType, ItemQuote->ItemID) : nullptr;
	}
}
