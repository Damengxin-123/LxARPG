// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterial.h"

/**
 * 物品静态数据缓存。
 *
 * 数据表管理器启动时会把各类物品数据表读入这里；运行时物品创建逻辑只通过这些接口查询，
 * 避免继续维护写死在代码里的物品常量 Map。
 */
namespace LxItemConfig
{
	void ClearItemConfig();

	void SetEquipmentItemData(const FLxEquipmentInformation& InItemData);
	void SetConsumableItemData(const FLxConsumableInformation& InItemData);
	void SetMaterialItemData(const FLxMaterialInformation& InItemData);
	void SetBuffItemData(const FLxBuffInformation& InItemData);

	const TMap<FLxItemID, FLxEquipmentInformation>& GetEquipmentItemMap();
	const TMap<FLxItemID, FLxConsumableInformation>& GetConsumableItemMap();
	const TMap<FLxItemID, FLxMaterialInformation>& GetMaterialItemMap();
	const TMap<FLxItemID, FLxBuffInformation>& GetBuffItemMap();

	/** 根据物品类型和 ID 查询物品静态配置。ItemID 不再承载类型信息，必须和类型配合定位。 */
	const FLxItemInformationBase* GetItemData(ELxItemType InItemType, FLxItemID InItemID);
	const FLxItemInformationBase* GetItemData(FGameplayTag InItemIDTag);
}
