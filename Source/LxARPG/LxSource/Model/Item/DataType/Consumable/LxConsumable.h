// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxConsumableCoreType.h"
#include "LxConsumableEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxConsumable.generated.h"


/**
 * @brief 消耗品物品定义类型
 * 该结构体继承自FLxItemDefineBase，用于定义游戏中消耗品类物品的属性。它包含了消耗品的核心信息和词条引用信息。
 *
 * @note 该类主要用于游戏内消耗品的数据定义，确保了消耗品属性的一致性和可配置性。
 */
USTRUCT(BlueprintType, DisplayName="消耗品物品定义类型")
struct FLxConsumableDefine : public FLxItemDefineBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品词条引用")
	FLxConsumableCoreType ConsumableCoreInfo;
	
	// 消耗品词条信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品词条引用")
	FLxConsumableEntryQuote ConsumableEntryInfo;
};

/**
 * @brief 消耗品物品缓存类型
 * 该结构体继承自FLxItemDateBase，用于存储游戏中消耗品类物品的详细数据。它包含了消耗品的核心信息和词条引用信息。
 *
 * @note 该类主要用于游戏内消耗品的数据缓存，确保了消耗品属性的一致性和可配置性。
 */
USTRUCT(BlueprintType, DisplayName="消耗品物品缓存类型")
struct FLxConsumableData : public FLxItemDateBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品词条引用")
	FLxConsumableCoreType ConsumableCoreInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品词条引用")
	FLxConsumableEntyInfo ConsumableEntryInfo;
};

