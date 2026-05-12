// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "UObject/Object.h"
#include "LxItemEnmuType.generated.h"

/**
 * @file LxItemEnmuType.h
 * @brief 物品相关枚举类型定义
 *
 * 此文件用于定义物品相关的枚举类型。
 */

/**
 * @enum ELxItemType
 * @brief 物品类型
 *
 * 定义了游戏中所有物品的大类类型。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType, DisplayName="物品类型")
enum class ELxItemType : uint8
{
	/** 其他类型 */
	None		UMETA(DisplayName = "其他类型"),
	/** 装备类型 */
	Equipment	UMETA(DisplayName = "装备"),
	/** 消耗品类型 */
	Consumable	UMETA(DisplayName = "消耗品"),
	/** 材料类型 */
	Material	UMETA(DisplayName = "材料"),
	/** 技能 */
	Skill		UMETA(DisplayName = "技能"),
	/** Buff */
	Buff		UMETA(DisplayName = "Buff"),
	
	/** 枚举最大值 - 此枚举表示物品类型有多少个类型 */
	ItemSizeMax UMETA(Hidden, DisplayName = "枚举最大值"),
};

// Item type tags, aligned with ELxItemType.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_None);
// 装备子类型
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Weapon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Deputy);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Helmet);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Armor);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Leggings);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Boots);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Glove);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Belt);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Equipment_Jewelry);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Consumable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Material);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Skill);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(LxTag_Item_Buff);

/**
 * @enum ELxItemUseState
 * @brief 物品使用后的效果
 *
 * 定义了物品被使用之后，应该进行的操作。
 */
enum ELxItemUseState
{
	// 使用失败：物品不能使用
	Failed,
	// 使用后需要安装装备：使用之后需要将此装备进行安装
	InstallEquipment,
	// 使用后需要卸载装备：使用之后需要将此装备进行卸载
	UnloadEquipment,
	// 使用后需要激活词条：使用后需要将此物品的词条进行激活
	ActivateEntry,
	// 使用后需要卸载buff：使用之后需要将此buff卸载
	UnloadBuff,
	// 使用后需要释放技能：使用之后，需要对此技能进行释放
	CastSkill,
};












