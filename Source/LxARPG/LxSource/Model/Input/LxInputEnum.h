#pragma once

#include "CoreMinimal.h"
#include "LxInputEnum.generated.h"

/**
 * @enum ELxInputValueAxial
 * @brief 轴类型枚举
 *
 * 该枚举定义了三维空间中的轴类型，包括X轴、Y轴和Z轴。
 * 用于标识与输入操作相关的轴方向。
 * 布尔类型的只直接使用X的值即可。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxInputValueAxial : uint8
{
	/** 无缩放 */
	None UMETA(DisplayName="无缩放"),
	/** X轴向 */
	X   UMETA(DisplayName="X轴向"),
	/** Y轴向 */
	Y	UMETA(DisplayName="Y轴向"),
	/** Z轴向 */
	Z	UMETA(DisplayName="Z轴向")
};

/**
 * @enum ELxInputInteractionType
 * @brief 输入交互类型枚举
 *
 * 该枚举定义了不同类型的输入交互方式，包括持续型、按下/松开成对和单次触发。
 * 用于描述用户与应用交互的具体行为模式。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxInputInteractionType : uint8
{
	/** 持续型（移动、视角） */
	Continuous			UMETA(DisplayName="持续型"),

	/** 按下 / 松开成对（跳跃、蓄力） */
	PressAndRelease		UMETA(DisplayName="按下/松开成对型"),

	/** 单次触发（打开 UI、确认） */
	SingleTrigger		UMETA(DisplayName="单次触发型")
};

