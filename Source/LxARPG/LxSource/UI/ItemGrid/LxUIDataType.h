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






