#pragma once

#include "LxAttributeEnumType.h"
#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxValueBase.h"


#include "LxAttributeSubData.generated.h"

/**
 * @brief 用于管理属性值的集合
 *
 * FLxAttributeValueSet 类设计用于处理属性值的集合，提供添加、删除和操作这些值的方法。它作为一个容器，允许高效地管理和访问属性，这些属性通常用于需要将多个属性或设置组合在一起的场景中。
 *
 * 该类特别适用于需要动态修改一组相关属性的应用程序中，例如游戏开发、配置管理和任何需要处理多个相互关联设置的系统中。
 */
USTRUCT(BlueprintType)
struct FLxAttributeValueSet
{
	GENERATED_BODY()

	/**
	 * @brief 最大值
	 *
	 * 该属性表示设置的最大值。默认情况下，最大值被初始化为一个错误标识符ERR_ATTRIBUTE。
	 * 可以在编辑器中进行修改，并且支持蓝图读写操作。
	 *
	 * @note ERR_ATTRIBUTE是一个预定义的错误标识符，其值为-9999999。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "属性值设置", DisplayName="最大值")
	int32 m_nMaxValue = ERR_ATTRIBUTE;

	/**
	 * @brief 最小值
	 *
	 * 该属性表示设置的最小值。默认情况下，最小值被初始化为一个错误标识符ERR_ATTRIBUTE。
	 * 可以在编辑器中进行修改，并且支持蓝图读写操作。
	 *
	 * @note ERR_ATTRIBUTE是一个预定义的错误标识符，其值为-9999999。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "属性值设置", DisplayName="最小值")
	int32 m_nMinValue = ERR_ATTRIBUTE;

	/**
	 * @brief 上浮动比例
	 *
	 * 默认浮动比例指在设计阶段设定的属性有效值浮动比例，
	 * 在使用属性值参与计算时，会在有效的值的上下浮动区间中取一个随机的值参与计算，
	 * 浮动比例指的就是这个浮动区间的上区间和下区间，
	 * 例如属性有效值为10， 浮动区间为20（即20%），选取参与计算的值便会在8~12之间（包括8，12），如果浮动比例为0，则视为不随机
	 *
	 * @note ERR_ATTRIBUTE是一个预定义的错误标识符，其值为-9999。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "属性值设置", DisplayName="上浮动比例")
	int32 m_nMaxFloatRatio = ERR_ATTRIBUTE;

	/**
	 * @brief 下浮动比例
	 *
	 * 默认浮动比例指在设计阶段设定的属性有效值浮动比例，
	 * 在使用属性值参与计算时，会在有效的值的上下浮动区间中取一个随机的值参与计算，
	 * 浮动比例指的就是这个浮动区间的上区间和下区间，
	 * 例如属性有效值为10， 浮动区间为20（即20%），选取参与计算的值便会在8~12之间（包括8，12），如果浮动比例为0，则视为不随机
	 *
	 * @note ERR_ATTRIBUTE是一个预定义的错误标识符，其值为-9999。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "属性值设置", DisplayName="下浮动比例")
	int32 m_nMinFloatRatio = ERR_ATTRIBUTE;

	bool IsValid() const
	{
		return m_nMaxValue != ERR_ATTRIBUTE || m_nMinValue != ERR_ATTRIBUTE ||
			m_nMaxFloatRatio != ERR_ATTRIBUTE || m_nMinFloatRatio != ERR_ATTRIBUTE;
	}
	FLxAttributeValueSet* operator=(const FLxAttributeValueSet& NewValue)
	{
		m_nMaxValue = NewValue.m_nMaxValue;
		m_nMinValue = NewValue.m_nMinValue;
		m_nMaxFloatRatio = NewValue.m_nMaxFloatRatio;
		m_nMinFloatRatio = NewValue.m_nMinFloatRatio;
		
		return this;
	}

	FLxAttributeValueSet operator*(const double Value) const
	{
		FLxAttributeValueSet NewValue = *this;
		if (IsValid())
		{
			if (NewValue.m_nMaxValue != ERR_ATTRIBUTE)
			{
				NewValue.m_nMaxValue = FMath::RoundToInt(NewValue.m_nMaxValue * Value);
			}
			if (NewValue.m_nMinValue != ERR_ATTRIBUTE)
			{
				NewValue.m_nMinValue = FMath::RoundToInt(NewValue.m_nMinValue * Value);
			}
			if (NewValue.m_nMaxFloatRatio != ERR_ATTRIBUTE)
			{
				NewValue.m_nMaxFloatRatio = FMath::RoundToInt(NewValue.m_nMaxFloatRatio * Value);
			}
			if (NewValue.m_nMinFloatRatio != ERR_ATTRIBUTE)
			{
				NewValue.m_nMinFloatRatio = FMath::RoundToInt(NewValue.m_nMinFloatRatio * Value);
			}
		}
		return NewValue;
	}

	FLxAttributeValueSet* operator*=(const double Value)
	{
		this->operator=(this->operator*(Value));
		return this;
	}
};

/**
 * @struct FLxBonusDataInfo
 * @brief 用于保存对属性的加成的值的结构体
 *
 * 该结构体用于存储和管理属性的各种加成值，包括直接增加、百分比加成和机制型值等。
 * 支持多种加成计算方式，用于角色属性的最终值计算。
 *
 * @note 可在蓝图中使用
 */
USTRUCT(BlueprintType)
struct FLxBonusDataInfo
{
	GENERATED_BODY()

	// 数值增加
	FLxAttributeValueSet m_fAdditiveValue;

	// 数值提高
	FLxAttributeValueSet m_fIncreasedPercent;

	// 数值总增
	TArray<FLxAttributeValueSet> m_fMorePercent;

	// 机制设置
	FLxAttributeValueSet ModifierTag;
	
	/**
	 * @brief 判断值是否为空
	 *
	 * 检查所有加成值是否都为默认值（0或空）。
	 *
	 * @return 如果所有值都为空则返回true，否则返回false
	 */
	bool IsEmpty() const
	{
		return m_fAdditiveValue.IsValid() || m_fIncreasedPercent.IsValid() || !m_fMorePercent.IsEmpty() || ModifierTag.IsValid();
	}

	/**
	 * @brief 赋值运算符重载
	 *
	 * 将另一个FLxBonusDataInfo对象的值赋值给当前对象。
	 *
	 * @param NewBonus 源加成数据对象
	 * @return 返回当前对象的指针
	 */
	FLxBonusDataInfo* operator=(const FLxBonusDataInfo& NewBonus)
	{
		m_fAdditiveValue = NewBonus.m_fAdditiveValue;
		m_fIncreasedPercent = NewBonus.m_fIncreasedPercent;
		ModifierTag = NewBonus.ModifierTag;

		m_fMorePercent.Empty();
		m_fMorePercent = NewBonus.m_fMorePercent;

		return this;
	}
};