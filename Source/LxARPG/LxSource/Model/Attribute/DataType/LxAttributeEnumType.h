/**
 * @file LxAttributeEnumType.h
 * @brief 角色属性相关枚举类型定义
 *
 * 此文件用于定义所有与角色属性相关的枚举类型。
 */

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxAttributeEnumType.generated.h"




/**
 * @enum ELxAttributeType
 * @brief 定义了属性类型的枚举类
 *
 * 用于区分不同的属性类型，每种属性类型都有一个对应的显示名称，可以在蓝图中使用。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxAttributeType : uint8
{
	/** 基础属性 */
	Basic_Att		UMETA(DisplayName="基础属性"),
	/** 攻击属性 */
	Atk_Att			UMETA(DisplayName="攻击属性"),
	/** 防御属性 */
	Def_Att			UMETA(DisplayName="防御属性"),
	/** 元素属性 */
	Ele_Att			UMETA(DisplayName="元素属性"),
	/** 信仰属性 */
	Bel_Att			UMETA(DisplayName="信仰属性"),
	/** 其他属性 */
	Other_Att		UMETA(DisplayName="其他属性"),
	/** 无属性 */
	None			UMETA(DisplayName="无属性"),
};

/**
 * @enum ELxCharacterRaceType
 * @brief 定义了角色种族类型的枚举类
 *
 * 该枚举用于表示游戏中的不同角色种族类型。每个种族类型都有一个对应的显示名称，可以在蓝图中使用。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxCharacterRaceType : uint8
{
	/** 无效属性 */
	None			UMETA(DisplayName="无效属性"),
	/** 人类 */
	Human			UMETA(DisplayName="人类"),
	/** 精灵 */
	Elves			UMETA(DisplayName="精灵"),
	/** 矮人 */
	Dwarves			UMETA(DisplayName="矮人"),
	/** 龙族 */
	Dragon			UMETA(DisplayName="龙族"),
	/** 猫人族 */
	Beastmen_1		UMETA(DisplayName="猫人族"),
	/** 兽人 */
	Beastmen_2		UMETA(DisplayName="兽人"),
};

/**
 * @enum ELxCharacterCampType
 * @brief 定义了角色阵营类型的枚举类
 *
 * 用于区分不同的角色阵营，该枚举类可以在蓝图中使用，以方便地标识和处理不同阵营的角色。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxCharacterCampType : uint8
{
	/** 无效阵营 */
	None			UMETA(DisplayName="无效阵营"),
	/** 不受伤害类型 */
	Invincible		UMETA(DisplayName="不受伤害类型"),
	/** 友好 */
	Friendly		UMETA(DisplayName="友好"),
	/** 中立 */
	Neutral			UMETA(DisplayName="中立"),
	/** 敌对 */
	Hostile			UMETA(DisplayName="敌对"),
};

/**
 * @enum ELxCharacterValueType
 * @brief 定义了角色属性值类型的枚举类
 *
 * 用于区分不同的角色属性类型，每种属性值类型都有一个对应的显示名称，可以在蓝图中使用。
 *
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxCharacterValueType : uint8
{
	/** 数值型属性 */
	FixedNumeric			UMETA(DisplayName="固定数值型"),
	FloatNumeric			UMETA(DisplayName="浮动数值型"),
	/** 百分比型 */
	FixedPercentage		UMETA(DisplayName="百分比型"),
	FloatPercentage		UMETA(DisplayName="浮动百分比型"),
	/** 机制型属性 */
	Mechanism		UMETA(DisplayName="机制型"),
};

/**
 * @enum ELxBiologicalAttributes
 * @brief 生物属性类型枚举
 *
 * 数值型属性的枚举。
 *
 * @note 作废，属性id已经改为直接使用字符串作为主键
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType)
enum class ELxBiologicalAttributes : uint8
{
	// 单位相关
	/** 种族 */
	BA_Race				  UMETA(DisplayName = "种族"),
	/** 阵营 */
	BA_Camp				  UMETA(DisplayName = "阵营"),

	// 基础属性
	/** 力量 */
    BA_Power              UMETA(DisplayName = "力量"),
	/** 敏捷 */
    BA_Agility            UMETA(DisplayName = "敏捷"),
	/** 智慧 */
    BA_Intelligence       UMETA(DisplayName = "智慧"),
	/** 体质 */
    BA_Constitution       UMETA(DisplayName = "体质"),



    // 攻击相关
	/** 物理攻击力 */
    BA_PhysicalAttack     UMETA(DisplayName = "物理攻击力"),
	/** 暴击几率 */
    BA_CriticalChance     UMETA(DisplayName = "暴击几率"),
	/** 暴击伤害 */
    BA_CriticalDamage     UMETA(DisplayName = "暴击伤害"),
	/** 魔力附加物理伤害 */
    BA_MagicAddPhysical   UMETA(DisplayName = "魔力附加物理伤害"),
	/** 魔力转换元素伤害 */
    BA_MagicToElement     UMETA(DisplayName = "魔力转换元素伤害"),
	/** 攻击速度 */
    BA_AttackSpeed        UMETA(DisplayName = "攻击速度"),
	/** 魔力回复 */
    BA_ManaRegen          UMETA(DisplayName = "魔力回复"),

    // 防御相关
	/** 护盾上限 */
    BA_ShieldMax          UMETA(DisplayName = "护盾上限"),
	/** 护盾充能速度 */
    BA_ShieldRechargeRate UMETA(DisplayName = "护盾充能速度"),
	/** 护盾流失速度 */
    BA_ShieldDecayRate    UMETA(DisplayName = "护盾流失速度"),
	/** 护甲防御力 */
    BA_ArmorDefense       UMETA(DisplayName = "护甲防御力"),
	/** 生命值 */
    BA_HP                 UMETA(DisplayName = "生命值"),
	/** 最大生命值 */
    BA_HPMax              UMETA(DisplayName = "最大生命值"),
	/** 魔力值 */
    BA_MP                 UMETA(DisplayName = "魔力值"),
	/** 最大魔力值 */
    BA_MPMax              UMETA(DisplayName = "最大魔力值"),
	/** 格挡减伤率 */
    BA_BlockReduction     UMETA(DisplayName = "格挡减伤率"),
	/** 体力 */
    BA_Stamina            UMETA(DisplayName = "体力"),

    // 元素亲和
	/** 火元素亲和 */
    BA_FireAffinity       UMETA(DisplayName = "火元素亲和"),
	/** 水元素亲和 */
    BA_WaterAffinity      UMETA(DisplayName = "水元素亲和"),
	/** 电元素亲和 */
    BA_ElectricAffinity   UMETA(DisplayName = "电元素亲和"),
	/** 光元素亲和 */
    BA_LightAffinity      UMETA(DisplayName = "光元素亲和"),
	/** 风元素亲和 */
    BA_WindAffinity       UMETA(DisplayName = "风元素亲和"),
	/** 土元素亲和 */
    BA_EarthAffinity      UMETA(DisplayName = "土元素亲和"),
	/** 木元素亲和 */
    BA_WoodAffinity       UMETA(DisplayName = "木元素亲和"),

    // 信仰亲和
	/** 光明之神信仰 */
    BA_LightGodFaith      UMETA(DisplayName = "光明之神信仰"),
	/** 自然之神信仰 */
    BA_NatureGodFaith     UMETA(DisplayName = "自然之神信仰"),
	/** 战争之神信仰 */
    BA_WarGodFaith        UMETA(DisplayName = "战争之神信仰"),
	/** 艺术之神信仰 */
    BA_ArtGodFaith        UMETA(DisplayName = "艺术之神信仰"),
	/** 命运之神信仰 */
    BA_FateGodFaith       UMETA(DisplayName = "命运之神信仰"),
	/** 财富之神信仰 */
    BA_WealthGodFaith     UMETA(DisplayName = "财富之神信仰"),
	/** 死亡之神信仰 */
    BA_DeathGodFaith      UMETA(DisplayName = "死亡之神信仰"),

    // 其他属性
	/** 负重 */
    BA_CarryWeight        UMETA(DisplayName = "负重"),
	/** 幸运 */
    BA_Luck               UMETA(DisplayName = "幸运"),

	// 最大值
	/** 最大值 */
	Ba_MaxValue			  UMETA(DisplayName = "最大值")
};

