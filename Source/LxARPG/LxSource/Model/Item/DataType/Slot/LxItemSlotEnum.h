// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemSlotEnum.generated.h"


/**
 * @enum ELxItemSlotType
 * @brief 容器类型
 *
 * 定义了游戏中不同类型的物品容器。
 *
 * @note 可在蓝图中使用
 */
// UENUM(BlueprintType, DisplayName="物品槽位类型")
// enum class ELxItemSlotType  : uint8
// {
// 	// 空类型
// 	None			UMETA(DisplayName = "其他类型"),
// 	// 背包容器
// 	Backpack		UMETA(DisplayName = "背包容器"),
// 	// 装备栏容器
// 	Equipment		UMETA(DisplayName = "装备栏容器"),
// 	// 仓库容器（可放入物品）
// 	Warehouse 		UMETA(DisplayName = "仓库容器（可放入物品）"),
// 	// 箱子容器（不可放入物品）
// 	TreasureChest 	UMETA(DisplayName = "箱子容器（不可放入物品）"),
// 	// 交易容器（不能交换位置）
// 	Transaction 	UMETA(DisplayName = "交易容器（不能交换位置）"),
// 	// 快捷方式（只进不出）
// 	Shortcut  		UMETA(DisplayName = "快捷方式（只进不出）"),
// 	BuffDisplay		UMETA(DisplayName = "Buff Display"),
// };

UENUM(BlueprintType, DisplayName="物品槽位归属")
enum class ELxItemSlotType : uint8
{
	// 无归属
	None			UMETA(DisplayName = "无归属"),
	// 背包格子
	Backpack		UMETA(DisplayName = "背包格子"),
	// 装备格子
	Equipment		UMETA(DisplayName = "装备格子"),
	// 仓库格子
	Warehouse		UMETA(DisplayName = "仓库格子"),
	// 宝箱格子
	TreasureChest	UMETA(DisplayName = "宝箱格子"),
	// 交易格子
	Transaction 	UMETA(DisplayName = "交易格子"),
	// 快捷格子
	Shortcut  		UMETA(DisplayName = "快捷格子"),
	// Buff格子
	BuffDisplay		UMETA(DisplayName = "Buff格子"),
	// 技能格子
	SkillDisplay		UMETA(DisplayName = "技能格子"),

};
UENUM(BlueprintType)
enum class ELxItemSlotDropResult : uint8
{
	// 交换成功 指双方进行了直接交换
	Swapped					UMETA(DisplayName="交换成功"),
	// 完全堆叠	值发起方物品完全堆叠到目标方
	StackedAll				UMETA(DisplayName="完全堆叠"),
	// 部分堆叠  指发起方物品堆叠到目标方之后，还剩了一些
	StackedPartial			UMETA(DisplayName="部分堆叠"),
	// 来源无效  指发起方没有物品
	FailedInvalidSource		UMETA(DisplayName="来源无效"),
	// 不可进入  指目标方不能放置发起方物品
	CannotEnter				UMETA(DisplayName="不可进入"),
	// 类型错误 指发起方与目标方槽位类型不一致 特指具体的物品类型，如装备部位
	TypeError				UMETA(DisplayName="不可进入"),
	// 进入成功  指发起方物品直接进入目标方空槽位
	EnterSuccess			UMETA(DisplayName="进入成功"),
};

