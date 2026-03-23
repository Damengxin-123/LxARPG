// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxConsumableEnum.h"
#include "LxARPG/LxSource/Model/Item/ItemBase/LxItemDataType.h"
#include "LxConsumableData.generated.h"



/**
 * @struct FLxConsumableData
 * @brief 表示消耗品类型物品的属性的结构体
 *
 * 继承自FLxItemAttributes，用于定义消耗品类型的属性，并在数据表中使用该结构体来存储消耗品数据。
 *
 * @note
 * - 本结构体用于定义消耗品类型的属性，并在数据表中使用该结构体来存储消耗品数据
 * - 本类型不与其他类型进行关联
 * - 本类型ID为行名称
 */
USTRUCT(BlueprintType, DisplayName="消耗品属性")
struct FLxConsumableData : public FLxItemBase
{
	GENERATED_BODY()

	/**
	 * @var ELxConsumableType ConsumableType
	 * @brief 消耗品类型
	 */
	ELxConsumableType ConsumableType = ELxConsumableType::Consumable;

	/**
	 * @brief 消耗品属性默认构造函数
	 *
	 * 初始化消耗品的默认属性值，设置物品类型为消耗品。
	 */
	FLxConsumableData()
	{
		ItemType = ELxItemType::Consumable;
	}

	virtual void InitData() override{};
	

	/**
	 * @brief 使用物品
	 *
	 * @return 如果使用成功则返回true，否则返回false
	 */
	virtual bool UseItem() override
	{
		if (ConsumableType == ELxConsumableType::NotConsumable)
		{
			return true;
		}
		ItemCount --;
		return true;
	};
};

