// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../ItemBase/LxItemBase.h"
#include "LxEquipment.generated.h"


USTRUCT(BlueprintType, DisplayName="装备基础属性信息")
struct FLxEquipmentInfo
{
	GENERATED_BODY()
	/**
	 * @var ELxEquipmentType EquipmentType
	 * @brief 装备部位类型
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="装备部位")
	ELxEquipmentType EquipmentType;

	/**
  	 * @var double StrengthenValue
  	 * @brief 装备的默认强化强度
  	 * 此值在计算时需要 / 100，表示百分比值
  	 * 百分比值，表示装备中所有属性的提高比例。
  	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="默认强化强度")
	int32 StrengthenValue = ERR_ATTRIBUTE;

	/**
	 * @var double ForgingPotential
	 * @brief 默认锻造潜能
	 * 此值在计算时需要 / 100，表示百分比值
	 * 通过锻造可以改变装备的扩展词条的数量及强度，但是会随机消耗锻造潜能。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="默认锻造潜能")
	int32 ForgingPotential = ERR_ATTRIBUTE;
};

USTRUCT(BlueprintType, DisplayName="装备词条引用信息")
struct FLxEquipmentEntyQuote
{
	GENERATED_BODY()
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "<UNK>", DisplayName="<UNK>")
	
};

/**
 * @brief 装备属性定义类型
 *
 * 该结构体继承自`FLxItemDefineBase`，用于定义装备的属性。它扩展了基础物品定义，以包含特定于装备的信息。
 * 可以在编辑器中设置，并且可以通过蓝图读写。
 *
 * @see FLxItemDefineBase
 */
USTRUCT(BlueprintType, DisplayName="装备属性定义类型")
struct FLxEquipmentDefine : public FLxItemDefineBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备属性", DisplayName="装备基础默认属性")
	FLxEquipmentInfo EquipmentInfo;
	
};
