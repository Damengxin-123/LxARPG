#pragma once

#include "CoreMinimal.h"
#include "LxEntryEnum.generated.h"

/**
 * 词条作用方式。
 * 用于描述词条数值如何参与属性计算。
 */
UENUM(BlueprintType, DisplayName="词条作用方式")
enum class ELxEntryEffectiveType : uint8
{
	// 加成基础值
	BasicValue			UMETA(DisplayName="加成基础值"),
	// 提高基础值
	BasicImprove		UMETA(DisplayName="提高基础值"),
	// 总增基础值
	AdditionalImprove	UMETA(DisplayName="总增基础值"),
	// 机制型
	Mechanism			UMETA(DisplayName="机制型"),
};

/**
 * 词条作用对象。
 * 指定属性类词条影响属性数值结构中的哪一个字段。
 */
UENUM(BlueprintType, DisplayName="词条作用对象")
enum class ELxEntryTarget : uint8
{
	// 有效值
	ToValue					UMETA(DisplayName="有效值"),
	// 值上限
	ToValueLimit			UMETA(DisplayName="值上限"),
	// 向上浮动比例
	ToUpwardFloatingRatio	UMETA(DisplayName="向上浮动比例"),
	// 向下浮动比例
	ToDownwardFloatingRatio	UMETA(DisplayName="向下浮动比例"),
};

using ELxItemEntryType = ELxEntryEffectiveType;
using ELxItemEntryTarget = ELxEntryTarget;

/**
 * 词条逻辑类型。
 * 用于描述物品修改、交互编辑等系统对物品上某条词条的处理规则。
 */
UENUM(BlueprintType, DisplayName="词条逻辑类型")
enum class ELxEntryLogicType : uint8
{
	// 普通词条：可在后续交互功能中被修改或删除。
	Normal	UMETA(DisplayName="普通词条"),
	// 基础词条：物品自带基础词条，玩家只能进行有限编辑，例如调整数值。
	Base	UMETA(DisplayName="基础词条"),
	// 锁定词条：无论位于何种物品上，都不可被编辑。
	Locked	UMETA(DisplayName="锁定词条"),
	// 特殊词条：具有特殊逻辑效果，例如不可强化、不可出售。
	Special	UMETA(DisplayName="特殊词条"),
};

/** 词条类型。用于区分运行时词条对象类型，不再作为词条数据主键。 */
UENUM(BlueprintType, DisplayName="词条类型")
enum class ELxEntryType : uint8
{
	// 无词条
	NoneEntryType		UMETA(DisplayName="无词条"),
	// 属性数值修改，统一承载能力值、判定、百分比和普通数值等标量修改。
	AttributeGain		UMETA(DisplayName="属性数值修改"),
	// 属性回复
	AttributeRecovery	UMETA(DisplayName="属性回复"),
	// 改变状态
	ChangeState			UMETA(DisplayName="改变状态"),
	// 创建 Buff
	CreateBuff			UMETA(DisplayName="创建Buff"),
	// 多目标
	MultiTarget			UMETA(DisplayName="多目标"),
	// 显示文本
	DisplayText			UMETA(DisplayName="显示文本"),
	// 授予技能
	GrantSkill			UMETA(DisplayName="授予技能"),
	// 赋予职业
	GrantProfession		UMETA(DisplayName="赋予职业"),
	// 造成伤害
	Damage				UMETA(DisplayName="造成伤害"),
	// 属性影响：使用一个属性字段按比例影响另一个属性字段。
	AttributeInfluence	UMETA(DisplayName="属性影响"),
};

/** 状态词条取值。 */
UENUM(BlueprintType, DisplayName="状态词条值")
enum class ELxEntryStateValue : uint8
{
	// 添加状态
	Add		UMETA(DisplayName="添加状态"),
	// 移除状态
	Remove	UMETA(DisplayName="移除状态"),
	// 切换状态
	Toggle	UMETA(DisplayName="切换状态"),
};
