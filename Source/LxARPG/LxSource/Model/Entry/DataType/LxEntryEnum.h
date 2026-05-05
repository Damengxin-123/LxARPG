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
 * 指定属性类词条影响属性数值结构中的哪个字段。
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

/** 词条类型。 */
UENUM(BlueprintType, DisplayName="词条类型")
enum class ELxEntryType : uint8
{
	// 无词条
	NoneEntryType		UMETA(DisplayName="无词条"),
	// 属性增益
	AttributeGain		UMETA(DisplayName="属性增益"),
	// 属性回复
	AttributeRecovery	UMETA(DisplayName="属性回复"),
	// 改变状态
	ChangeState			UMETA(DisplayName="改变状态"),
	// 创建Buff
	CreateBuff			UMETA(DisplayName="创建Buff"),
	// 多目标
	MultiTarget			UMETA(DisplayName="多目标"),
	// 显示文本
	DisplayText			UMETA(DisplayName="显示文本"),
};

/** 更改状态词条的取值。 */
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

/** 属性增益词条 ID。 */
UENUM(BlueprintType, DisplayName="属性增益词条ID")
enum class ELxAttributeGainEntryID : uint8
{
	None			UMETA(DisplayName="无"),
	GainStrength_1	UMETA(DisplayName="力量增加1"),
	GainWisdom_1	UMETA(DisplayName="智慧增加1"),
	GainMaxHP_1		UMETA(DisplayName="生命上限增加1"),
};

/** 属性回复词条 ID。 */
UENUM(BlueprintType, DisplayName="属性回复词条ID")
enum class ELxAttributeRecoveryEntryID : uint8
{
	None				UMETA(DisplayName="无"),
	RecoverHP_1			UMETA(DisplayName="生命回复1"),
	RecoverHPPercent_1	UMETA(DisplayName="生命百分比回复1"),
};

/** 状态改变词条 ID。 */
UENUM(BlueprintType, DisplayName="状态改变词条ID")
enum class ELxChangeStateEntryID : uint8
{
	None				UMETA(DisplayName="无"),
	ImmuneAllDamage_1	UMETA(DisplayName="免疫所有伤害1"),
	RecoveringHP_1		UMETA(DisplayName="正在恢复生命1"),
};

/** 创建 Buff 词条 ID。 */
UENUM(BlueprintType, DisplayName="创建Buff词条ID")
enum class ELxCreateBuffEntryID : uint8
{
	None					UMETA(DisplayName="无"),
	RestoreHPBuff_1			UMETA(DisplayName="创建生命回复Buff1"),
	GainStrengthBuff_1		UMETA(DisplayName="创建力量提高Buff1"),
};

/** 多目标词条 ID。 */
UENUM(BlueprintType, DisplayName="多目标词条ID")
enum class ELxMultiTargetEntryID : uint8
{
	None UMETA(DisplayName="无"),
};

/** 显示文本词条 ID。 */
UENUM(BlueprintType, DisplayName="显示文本词条ID")
enum class ELxDisplayTextEntryID : uint8
{
	None					UMETA(DisplayName="无"),
	DisplayRare_1			UMETA(DisplayName="十分稀有1"),
	DisplayUndroppable_1	UMETA(DisplayName="不可丢弃1"),
};
