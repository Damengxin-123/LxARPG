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

	// 固定数值型  直接使用有效数值的属性类型，例如 力量值 15
	FixedNumeric			UMETA(DisplayName="固定数值型"),
	// 数值区间型  有效值下限为0，上限受最大值限制的值的类型，在使用时需要最大值和有效值一起使用，例如 生命值  500/1000
	RangedNumeric			UMETA(DisplayName="数值区间型"),
	// 数值浮动型  有效数值在一个上下区间内随机取值的类型，在使用时需要在区间内随机取一个值，例如 攻击力 8~12
	FloatingNumeric		UMETA(DisplayName="数值浮动型"),
	// 概率型	 有效值为一个触发某种机制的概率 例如 暴击率 20%
	Probabilistic		UMETA(DisplayName="概率型"),
	// 百分比型	 有效值为一个百分比数值的类型，例如 格挡减伤率  45%
	Percentage			UMETA(DisplayName="百分比型"),
	// 开关型	 有效值为布尔类型的类型 例如 是否免疫某种伤害 是、否
	Switch				UMETA(DisplayName="开关型"),
	// 设定型	 有效值为某种类似于枚举的类型 例如 种族 0=人族
	Setting				UMETA(DisplayName="设定型"),
};

/**
 * @enum ELxCharacterAttributeID
 * @brief 生物属性类型枚举
 *
 * 数值型属性的枚举。
 *
 * @note 
 * @note 可在蓝图中使用
 */
UENUM(BlueprintType, DisplayName="角色属性ID")
enum class ELxCharacterAttributeID : uint8
{
	X_None				  UMETA(DisplayName="空ID"),
	/** 角色单位属性 */
	/** 种族 */
	A_Race				  UMETA(DisplayName = "种族"),
	/** 阵营 */
	A_Camp				  UMETA(DisplayName = "阵营"),
	/** 负重 */
	A_CarryWeight         UMETA(DisplayName = "负重"),
	/** 幸运 */
	A_Luck                UMETA(DisplayName = "幸运"),
	// 角色单位属性最大值
	A_Max				  UMETA(DisplayName = "角色单位属性最大值", Hidden),

	// 角色基础属性
	/** 力量 */
    B_Power              UMETA(DisplayName = "力量"),
	/** 敏捷 */
    B_Agility            UMETA(DisplayName = "敏捷"),
	/** 智慧 */
    B_Intelligence       UMETA(DisplayName = "智慧"),
	/** 体质 */
    B_Constitution       UMETA(DisplayName = "体质"),
	// 基础属性最大值
	B_Max				 UMETA(DisplayName = "基础属性最大值", Hidden),

	// 资源相关属性
	// 生命值
	C_HP				UMETA(DisplayName = "生命值"),
	// 魔力值
	C_MP				UMETA(DisplayName = "魔力值"),
	// 体力值
	C_Stamina			UMETA(DisplayName = "体力值"),
	// 资源相关属性最大值
	C_Max				UMETA(DisplayName = "资源相关属性最大值", Hidden),

    // 攻击相关属性
	/** 物理攻击力 */
    D_PhysicalAttack     UMETA(DisplayName = "攻击力"),
	/** 暴击几率 */
    D_CriticalChance     UMETA(DisplayName = "暴击几率"),
	/** 暴击伤害 */
    D_CriticalDamage     UMETA(DisplayName = "暴击伤害"),
	/** 攻击速度 */
    D_AttackSpeed        UMETA(DisplayName = "攻击速度"),
	// 攻击相关属性最大值
	D_Max				 UMETA(DisplayName = "攻击相关属性最大值", Hidden),

    // 防御相关
	// 防御力
	E_Defense			UMETA(DisplayName = "防御力"),
	// 护盾
	E_Shield			UMETA(DisplayName = "护盾"),
	// 防御相关属性最大值
	E_Max				UMETA(DisplayName = "防御相关属性最大值", Hidden),

	// 元素相关
	// 火元素亲和
	F_Fire				UMETA(DisplayName = "火元素亲和"),
	// 
	F_Water				UMETA(DisplayName = "水元素亲和"),
	// 电元素亲和
	F_Electric			UMETA(DisplayName = "电元素亲和"),
	// 元素相关属性最大值
	F_Max				UMETA(DisplayName = "元素相关属性最大值", Hidden),

	// 信仰相关属性
	// 光明之神信仰
	G_LightGod			UMETA(DisplayName = "光明之神信仰"),
	// 自然之神信仰
	G_Nature			UMETA(DisplayName = "自然之神信仰"),
	// 信仰相关属性最大值
	G_Max				UMETA(DisplayName = "信仰相关属性最大值", Hidden),
    // 信仰亲和
	// /** 光明之神信仰 */
 //    BA_LightGodFaith      UMETA(DisplayName = "光明之神信仰"),
	// /** 自然之神信仰 */
 //    BA_NatureGodFaith     UMETA(DisplayName = "自然之神信仰"),
	// /** 战争之神信仰 */
 //    BA_WarGodFaith        UMETA(DisplayName = "战争之神信仰"),
	// /** 艺术之神信仰 */
 //    BA_ArtGodFaith        UMETA(DisplayName = "艺术之神信仰"),
	// /** 命运之神信仰 */
 //    BA_FateGodFaith       UMETA(DisplayName = "命运之神信仰"),
	// /** 财富之神信仰 */
 //    BA_WealthGodFaith     UMETA(DisplayName = "财富之神信仰"),
	// /** 死亡之神信仰 */
 //    BA_DeathGodFaith      UMETA(DisplayName = "死亡之神信仰"),


	// 最大值
	/** 最大值 */
	Ba_MaxValue			  UMETA(DisplayName = "最大值")
};
