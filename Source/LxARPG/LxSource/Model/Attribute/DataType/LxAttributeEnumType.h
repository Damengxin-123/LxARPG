#pragma once

#include "CoreMinimal.h"
#include "LxAttributeEnumType.generated.h"

/**
 * 属性类型
 */
UENUM(BlueprintType, DisplayName = "属性类型")
enum class ELxAttributeType : uint8
{
	Basic_Att UMETA(DisplayName = "基础属性"),
	Vital_Att UMETA(DisplayName = "生存属性"),
	Combat_Att UMETA(DisplayName = "战斗属性"),
	Skill_Att UMETA(DisplayName = "技能属性"),
	Equipment_Att UMETA(DisplayName = "装备属性"),
	Profession_Att UMETA(DisplayName = "职业属性"),
	Resource_Att UMETA(DisplayName = "资源属性")
};

/**
 * 角色种族类型
 */
UENUM(BlueprintType, DisplayName = "角色种族类型")
enum class ELxCharacterRaceType : uint8
{
	None UMETA(DisplayName = "无效"),
	Human UMETA(DisplayName = "人类"),
	Elves UMETA(DisplayName = "精灵"),
	Dwarves UMETA(DisplayName = "矮人"),
	Dragon UMETA(DisplayName = "龙族"),
	Beastmen_1 UMETA(DisplayName = "兽人1"),
	Beastmen_2 UMETA(DisplayName = "兽人2")
};

/**
 * 角色阵营类型
 */
UENUM(BlueprintType, DisplayName = "角色阵营类型")
enum class ELxCharacterCampType : uint8
{
	None UMETA(DisplayName = "无效"),
	Invincible UMETA(DisplayName = "无敌"),
	Friendly UMETA(DisplayName = "友好"),
	Neutral UMETA(DisplayName = "中立"),
	Hostile UMETA(DisplayName = "敌对")
};

/**
 * 角色属性值类型
 */
UENUM(BlueprintType, DisplayName = "角色属性值类型")
enum class ELxCharacterValueType : uint8
{
	FixedNumeric UMETA(DisplayName = "固定数值"),
	RangedNumeric UMETA(DisplayName = "区间数值"),
	FloatingNumeric UMETA(DisplayName = "浮动数值"),
	Probabilistic UMETA(DisplayName = "概率数值"),
	Percentage UMETA(DisplayName = "百分比数值"),
	Switch UMETA(DisplayName = "开关"),
	Setting UMETA(DisplayName = "设置")
};
