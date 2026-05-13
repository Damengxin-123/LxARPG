// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterial.h"

/**
 * Item static data cache.
 *
 * Data tables are loaded into tag-keyed maps so runtime systems can create,
 * stack, and query items without depending on legacy numeric IDs.
 */
namespace LxItemConfig
{
	void ClearItemConfig();

	void SetEquipmentItemData(const FLxEquipmentInformation& InItemData);
	void SetConsumableItemData(const FLxConsumableInformation& InItemData);
	void SetMaterialItemData(const FLxMaterialInformation& InItemData);
	void SetBuffItemData(const FLxBuffInformation& InItemData);

	const TMap<FGameplayTag, FLxEquipmentInformation>& GetEquipmentItemMap();
	const TMap<FGameplayTag, FLxConsumableInformation>& GetConsumableItemMap();
	const TMap<FGameplayTag, FLxMaterialInformation>& GetMaterialItemMap();
	const TMap<FGameplayTag, FLxBuffInformation>& GetBuffItemMap();

	/** Query item static data by its stable tag ID. */
	const FLxItemInformationBase* GetItemData(FGameplayTag InItemIDTag);
}
