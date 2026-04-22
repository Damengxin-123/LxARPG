// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxEquipmentEnum.h"
#include "../ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxEquipmentCoreType.generated.h"


USTRUCT(BlueprintType, DisplayName="装备基础属性信息")
struct FLxEquipmentInfo
{
	GENERATED_BODY()
	/**
	 * @var ELxEquipmentType EquipmentType
	 * @brief 装备部位类型
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="装备部位")
	ELxEquipmentType EquipmentType = ELxEquipmentType::Weapon;

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
	/**
	 * @var FLxItemEntryQuote EquipmentBasicEntryQuote
	 * @brief 装备默认词条引用
	 * 该属性用于定义装备的基本词条引用，通过引用词条定义表中的特定行来确定装备的基础属性。此属性支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作。
	 * 引用的词条定义包含了具体的属性信息和行为规则，确保了装备基础属性的一致性和可配置性。
	 *
	 * @note 确保所引用的词条定义存在并且是有效的，以避免运行时可能出现的错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备默认词条引用")
	FLxItemEntryQuote EquipmentBasicEntryQuote;

	/**
	 * @var TArray<FLxItemEntryQuote> EquipmentExtendEntryQuote
	 * @brief 装备扩展词条引用
	 * 该属性用于定义装备的额外扩展词条引用，通过引用词条定义表中的特定行来确定装备的附加属性。此属性支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作。
	 * 引用的扩展词条定义包含了具体的附加属性信息和行为规则，确保了装备扩展属性的一致性和可配置性。
	 *
	 * @note 确保所引用的扩展词条定义存在并且是有效的，以避免运行时可能出现的错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备扩展词条引用")
	TArray<FLxItemEntryQuote> EquipmentExtendEntryQuote;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="扩展词条最大数量")
	uint8 EquipmentExtendEntryMaxCount = 0;
	
};

/**
 * @struct FLxEquipmentEntyInfo
 * @brief 装备词条信息
 *
 * 该结构体用于存储装备的默认词条和扩展词条列表。通过编辑器或蓝图可以对其进行读写操作。
 * 在编辑器中，该变量显示为“装备词条信息”。
 *
 * @note 确保所有引用的词条定义存在并且有效，以避免运行时错误。
 */
USTRUCT(BlueprintType, DisplayName="装备词条信息")
struct FLxEquipmentEntyInfo
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备默认词条")
	FLxItemEntryData EquipmentBasicEntry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备扩展词条")
	TArray<FLxItemEntryData> EquipmentExtendEntryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="扩展词条最大数量")
	uint8 EquipmentExtendEntryMaxCount = 0;
};

