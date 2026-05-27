// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxItemSlotEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxItemSlotDataType.generated.h"

/**
 * @brief 一个结构体，用于定义物品槽位的逻辑和约束。
 */
USTRUCT()
struct FLxSlotLogicSet
{
	GENERATED_BODY()

	// 拖动物品进入 false：不能拖进来 true：可以拖进来
	bool ItemEnter = true;
	// 拖动物品离开 false：不能拖出去 true：可以拖出去
	bool ItemLeave = true;
	// 物品进入后是否为快捷方式 快捷方式指只引用物品指针，不清空原有物品槽位内物品指针
	bool ItemIsShortcut = false;

	FGameplayTag ItemTypeTag = LxTag_Item;

	FLxSlotLogicSet()
	{
		ItemEnter = true;
		ItemLeave = true;
		ItemIsShortcut = false;
		ItemTypeTag = LxTag_Item;
	};
	FLxSlotLogicSet(bool InItemEnter,  bool InItemLeave, bool InItemIsShortcut, FGameplayTag InItemTypeTag = LxTag_Item)
	: ItemEnter(InItemEnter), ItemLeave(InItemLeave),  ItemIsShortcut(InItemIsShortcut),  ItemTypeTag(InItemTypeTag)
	{};
	FLxSlotLogicSet(FGameplayTag InItemTypeTag) : ItemTypeTag(InItemTypeTag) {}
};

/**
 * @brief 一个常量映射表，用于根据不同的物品槽位类型定义其逻辑和约束。
 *
 * 该映射表通过 `ELxItemSlotType_Test` 枚举值作为键，对应的 `FLxSlotLogicSet` 结构体实例作为值，来定义不同槽位的规则。这些规则包括是否允许物品进入、离开槽位，以及进入后的物品是否为快捷方式等属性。
 */
const TMap<ELxItemSlotType, FLxSlotLogicSet> ConstItemSlotLogicSetMap = {
	{ELxItemSlotType::Backpack, FLxSlotLogicSet()},
	{ELxItemSlotType::Equipment, FLxSlotLogicSet(LxTag_Item_Equipment)},
	{ELxItemSlotType::Warehouse, FLxSlotLogicSet()},
	{ELxItemSlotType::TreasureChest, FLxSlotLogicSet(false, true, false)},
	{ELxItemSlotType::Transaction, FLxSlotLogicSet()},
	{ELxItemSlotType::Shortcut, FLxSlotLogicSet(true, false, true)},
	{ELxItemSlotType::BuffDisplay, FLxSlotLogicSet(false, false, false, LxTag_Item_Buff)},
	{ELxItemSlotType::BuffDisplay, FLxSlotLogicSet(false, false, false, LxTag_Item_Buff)},
	{ELxItemSlotType::SkillDisplay, FLxSlotLogicSet(false, true, false, LxTag_Item_Skill)},
};
