#pragma once

#include "CoreMinimal.h"
#include "LxInputEnum.generated.h"

UENUM(BlueprintType, DisplayName="输入行为ID")
enum class ELxInputActionID : uint8
{
	None UMETA(DisplayName="无"),

	ShowCursor  UMETA(DisplayName="显示鼠标光标"),
	
	MoveForward UMETA(DisplayName="前进"),
	MoveBackward UMETA(DisplayName="后退"),
	MoveLeft UMETA(DisplayName="向左"),
	MoveRight UMETA(DisplayName="向右"),
	Jump UMETA(DisplayName="跳跃"),
	LookX UMETA(DisplayName="视角X"),
	LookY UMETA(DisplayName="视角Y"),

	Backpack UMETA(DisplayName="背包"),
	CharacterAttribute UMETA(DisplayName="角色属性"),
	SkillBackpack UMETA(DisplayName="技能面板"),

	
	Shortcut0 UMETA(DisplayName="快捷栏0"),
	Shortcut1 UMETA(DisplayName="快捷栏1"),
	Shortcut2 UMETA(DisplayName="快捷栏2"),
	Shortcut3 UMETA(DisplayName="快捷栏3"),
	Shortcut4 UMETA(DisplayName="快捷栏4"),
	Shortcut5 UMETA(DisplayName="快捷栏5"),
	Shortcut6 UMETA(DisplayName="快捷栏6"),
	Shortcut7 UMETA(DisplayName="快捷栏7"),
	Shortcut8 UMETA(DisplayName="快捷栏8"),
	Shortcut9 UMETA(DisplayName="快捷栏9"),

	InteractionInteract UMETA(DisplayName="交互"),
	InteractionSelectCandidate UMETA(DisplayName="选择交互候选"),
	InteractionCancel UMETA(DisplayName="取消交互"),
	InteractionBack UMETA(DisplayName="返回上级交互"),
	SystemShowMouseCursor UMETA(DisplayName="显示鼠标")
};

FORCEINLINE uint32 GetTypeHash(const ELxInputActionID InInputActionID)
{
	return GetTypeHash(static_cast<uint8>(InInputActionID));
}

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

