/**
 * @file LxUIDataType.h
 * @brief UI数据类型定义
 *
 * 此文件定义了UI系统相关的枚举类型和宏定义。
 */

#pragma once

#include "CoreMinimal.h"
#include "LxUIDataType.generated.h"

/**
 * @enum EUIInputType
 * @brief 控制器对UI界面的控制枚举类型
 *
 * 定义了控制器对UI界面的各种控制行为。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class EUIInputType : uint8
{
	/** 无 */
	EIT_None UMETA(DisplayName = "无"),
	/** 关闭角色当前打开的焦点UI */
	CloseCurrentUI UMETA(DisplayName = "关闭当前焦点界面"),
	/** 对角色信息UI界面控制行为 */
	OpenCharacterInformationUI UMETA(DisplayName = "角色信息界面"),
	/** 对角色背包UI控制行为 */
	OpenBackpackUI UMETA(DisplayName = "打开背包界面"),

};

/**
 * @enum EItemSlotWidgetType
 * @brief 物品格子功能类型
 *
 * 定义了不同类型的物品格子，用于控制物品的存储和显示方式。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class EItemSlotWidgetType : uint8
{
	/** 无效枚举，一般用于初始化 */
	EIT_None		UMETA(DisplayName = "无"),
	/** 标准物品栏格子，用于直接提供物品显示和交互功能 */
	Inventory		UMETA(DisplayName = "物品格子"),
	/** 快捷物品栏，可以交互，但实际上是背包中物品格子的快捷方式 */
	Shortcut		UMETA(DisplayName = "快捷格子"),
	/** 物品类型限定为装备的物品格子，当类型不符合时，无法将物品设置到此格子 */
	Equipment		UMETA(DisplayName = "装备格子"),
	/** 物品类型限定为技能的物品格子，当类型不符合时，无法将物品设置到此格子 */
	Skill			UMETA(DisplayName = "技能格子"),
	/** 来自非玩家角色的物品的格子，在拖动时不能拖动到 Shortcut 类型中*/
	Warehouse		UMETA(DisplayName = "仓库格子"),


};




