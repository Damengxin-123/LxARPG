// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuffEnum.h"
#include "UObject/Object.h"
#include "LxBuffCoreType.generated.h"

/**
 * @brief 定义了Buff的基本信息，用于在游戏设计中表示不同类型的Buff。
 *
 * 该结构体主要用于存储和管理游戏中的Buff类型信息。通过`ELxBuffType`枚举来指定具体的Buff类型，从而实现对游戏中角色或单位所受增益或减益效果的分类管理。
 *
 * @note 该结构体支持蓝图编辑器内可视化编辑以及脚本读写操作。
 */
USTRUCT(BlueprintType, DisplayName="Buff定义类型")
struct FBuffCoreInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff类型")
	ELxBuffType  BuffType;
};

