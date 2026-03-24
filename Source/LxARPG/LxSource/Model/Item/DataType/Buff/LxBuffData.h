// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuffEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemDataType.h"
#include "UObject/Object.h"
#include "LxBuffData.generated.h"

/**
 * @brief FLxBuffAttributes 结构体用于表示游戏中的Buff属性。
 * 该结构体继承自FLxItemBase，提供了基础的物品属性，并添加了特定于Buff的类型信息。
 * Buff类型的定义通过ELxBuffType枚举来实现，允许设置和区分不同的Buff效果。
 */
USTRUCT(BlueprintType)
struct FLxBuffData : public FLxItemBase
{
	GENERATED_BODY()

	ELxBuffType BuffType = ELxBuffType::None;

	FLxBuffData()
	{
		ItemType = ELxItemType::Buff;
	}

	virtual void InitData() override{};
};
