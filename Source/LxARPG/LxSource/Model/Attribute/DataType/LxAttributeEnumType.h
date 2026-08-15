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
 * 角色属性业务分类。
 * 该分类只描述属性在玩法中的职责，不决定属性采用标量、资源或区间中的哪种存储结构。
 */
UENUM(BlueprintType, DisplayName="角色属性业务分类")
enum class ELxCharacterAttributeBusinessCategory : uint8
{
	Ability UMETA(DisplayName="能力值属性"),
	Resource UMETA(DisplayName="资源属性"),
	Judgement UMETA(DisplayName="判定属性"),
	Combat UMETA(DisplayName="战斗属性"),
	Action UMETA(DisplayName="行动属性"),
	Loot UMETA(DisplayName="掉落属性"),
	Element UMETA(DisplayName="元素属性"),
	Faith UMETA(DisplayName="信仰属性")
};

/** 角色属性数值结构类型，只描述运行时数据形态。 */
UENUM(BlueprintType, DisplayName="角色属性数值结构类型")
enum class ELxCharacterAttributeValueType : uint8
{
	Scalar UMETA(DisplayName="标量属性"),
	Resource UMETA(DisplayName="资源属性"),
	Range UMETA(DisplayName="区间属性")
};

/** 标量属性在界面中的数值格式。 */
UENUM(BlueprintType, DisplayName="标量属性显示格式")
enum class ELxScalarAttributeDisplayFormat : uint8
{
	Integer UMETA(DisplayName="整数"),
	Decimal UMETA(DisplayName="小数"),
	Percentage UMETA(DisplayName="百分比")
};
