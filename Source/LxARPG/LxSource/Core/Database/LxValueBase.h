// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxValueBase.generated.h"

using ValueType = int32;

/**
 * @def ERR_ATTRIBUTE
 * @brief 错误的属性值
 *
 * 用于异常返回值等错误情况的标识。
 */
# define ERR_ATTRIBUTE -9999

/**
 * @class FLxValueBase
 * @brief 值类型的基类，为各种数据表示提供基础。
 *
 * 该类设计为系统中不同值类型的基类。它封装了不同值类型之间共享的通用功能和行为。子类应扩展此类以实现具有自己特性和方法的具体值类型。
 * 注：
 *		1、当数值被double数值进行乘法或除法运算时，结果值会进行四舍五入处理，最后结果一定是整数
 * 
 * FLxValueBase 的主要目的是确保所有值类型具有一致的接口和行为，从而便于在应用程序中更轻松地管理和操作值。
 */
USTRUCT(BlueprintType)
struct FLxValueBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="数值", Category="数值设置")
	ValueType m_nValue = ERR_ATTRIBUTE;

	FLxValueBase* operator=(const FLxValueBase& NewValue)
	{
		m_nValue = NewValue.m_nValue;
		return this;
	}
	FLxValueBase* operator=(const ValueType NewValue)
	{
		m_nValue = NewValue;
		return this;
	}
	bool operator==(const FLxValueBase& InValue) const
	{
		return m_nValue == InValue.m_nValue;
	}
	bool operator==(const ValueType InValue) const
	{
		return m_nValue == InValue;
	}
	bool operator!=(const FLxValueBase& InValue) const
	{
		return m_nValue != InValue.m_nValue;
	}
	bool operator!=(const ValueType InValue) const
	{
		return m_nValue != InValue;
	}
	FLxValueBase* operator+=(const FLxValueBase& InValue)
	{
		m_nValue += InValue.m_nValue;
		return this;
	}
	FLxValueBase* operator+=(const ValueType InValue)
	{
		m_nValue += InValue;
		return this;
	}
	FLxValueBase* operator-=(const FLxValueBase& InValue)
	{
		m_nValue -= InValue.m_nValue;
		return this;
	}
	FLxValueBase* operator-=(const ValueType InValue)
	{
		m_nValue -= InValue;
		return this;
	}
	FLxValueBase* operator*=(const FLxValueBase& InValue)
	{
		m_nValue *= InValue.m_nValue;
		return this;
	}
	FLxValueBase* operator*=(const ValueType InValue)
	{
		m_nValue *= InValue;
		return this;
	}
	FLxValueBase* operator/=(const FLxValueBase& InValue)
	{
		m_nValue /= InValue.m_nValue;
		return this;
	}
	FLxValueBase* operator/=(const ValueType InValue)
	{
		m_nValue /= InValue;
		return this;
	}
	ValueType operator+(const FLxValueBase& InValue) const
	{
		return m_nValue + InValue.m_nValue;
	}
	ValueType operator+(const ValueType InValue) const
	{
		return m_nValue + InValue;
	}
	ValueType operator-(const FLxValueBase& InValue) const
	{
		return m_nValue - InValue.m_nValue;
	}
	ValueType operator-(const ValueType InValue) const
	{
		return m_nValue - InValue;
	}
	ValueType operator*(const FLxValueBase& InValue) const
	{
		return m_nValue * InValue.m_nValue;
	}
	ValueType operator*(const ValueType InValue) const
	{
		return m_nValue * InValue;
	}
	ValueType operator*(const double InValue) const
	{
		return FMath::RoundToInt(m_nValue * InValue);
	}
	ValueType operator/(const FLxValueBase& InValue) const
	{
		return m_nValue / InValue.m_nValue;
	}
	ValueType operator/(const ValueType InValue) const
	{
		return m_nValue / InValue;
	}
	ValueType operator/(const double InValue) const
	{
		return FMath::RoundToInt(m_nValue * InValue);
	}
	
};
