// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"

class LXARPG_API FLxAttributeValueTool
{

public:
	/**
 	 * @brief 计算给定属性值的最小浮动数值
 	 *
 	 * 该函数根据传入的FLxAttributeValue对象，计算并返回其基于向下浮动比例和向上浮动比例调整后的最小可能数值。
 	 *
 	 * @param AttributeValue 需要计算最小浮动数值的属性值对象
 	 * @return 返回计算得到的最小浮动数值
 	 */
	static int32 GetFloatingValueMin(const FLxAttributeValue& AttributeValue);
	/**
	 * @brief 计算给定属性值的最大浮动数值
	 *
	 * 该函数根据传入的FLxAttributeValue对象，计算并返回其基于向下浮动比例和向上浮动比例调整后的最大可能数值。
	 *
	 * @param AttributeValue 需要计算最大浮动数值的属性值对象
	 * @return 返回计算得到的最大浮动数值
	 */
	static int32 GetFloatingValueMax(const FLxAttributeValue& AttributeValue);
	/**
	 * @brief 根据给定的属性数据构建属性值的文本表示
	 *
	 * 该函数根据传入的FLxAttributeData对象，生成并返回一个描述属性值的FText对象。
	 * 生成的文本会根据属性值的类型（如范围数值、浮动数值、百分比等）进行格式化。
	 *
	 * @param AttributeData 包含需要转换为文本形式的属性数据的对象
	 * @return 返回一个FText对象，表示属性值的文本描述
	 */
	static FText BuildAttributeValueText(const FLxAttributeData& AttributeData);
};
