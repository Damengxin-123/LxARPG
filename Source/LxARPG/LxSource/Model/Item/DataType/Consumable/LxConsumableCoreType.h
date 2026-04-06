// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxConsumableEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxConsumableCoreType.generated.h"


/**
 * @brief 消耗品词条信息
 * 该结构体用于定义消耗品的词条信息，通过包含多个 `FLxItemEntryData` 条目来描述消耗品的各种属性。此结构体支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作。
 * `ConsumableEntryList` 包含了具体的属性信息和行为规则，确保了消耗品属性的一致性和可配置性。
 *
 * @note 确保 `ConsumableEntryList` 中的每个条目都是有效的，以避免运行时可能出现的错误。
 */
USTRUCT(BlueprintType, DisplayName="消耗品词条信息")
struct FLxConsumableEntyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品词条配置")
	TArray<FLxItemEntryData> ConsumableEntryList;
};

/**
 * @brief 消耗品词条引用信息
 * 该结构体用于定义消耗品的词条引用信息，通过引用词条定义表中的特定行来确定消耗品的具体属性。此属性支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作。
 * 引用的词条定义包含了具体的属性信息和行为规则，确保了消耗品属性的一致性和可配置性。
 *
 * @note 确保所引用的词条定义存在并且是有效的，以避免运行时可能出现的错误。
 */
USTRUCT(BlueprintType, DisplayName="消耗品词条引用信息")
struct  FLxConsumableEntryQuote
{
	GENERATED_BODY()
	/**
 	 * @var TArray<FLxItemEntryQuote> EquipmentExtendEntryQuote
 	 * @brief 装备扩展词条引用
 	 * 该属性用于定义装备的额外扩展词条引用，通过引用词条定义表中的特定行来确定装备的附加属性。此属性支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作。
 	 * 引用的扩展词条定义包含了具体的附加属性信息和行为规则，确保了装备扩展属性的一致性和可配置性。
 	 *
 	 * @note 确保所引用的扩展词条定义存在并且是有效的，以避免运行时可能出现的错误。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品词条引用")
	TArray<FLxItemEntryQuote> ConsumableEntryQuote;
};

/**
 * @brief 消耗品基础信息
 * 该结构体用于定义消耗品的基本类型信息。通过 `ConsumableType` 属性来区分不同类型的消耗品，例如可消耗型和不可消耗型。
 * 此结构体支持在编辑器中进行可视化编辑，并且可以通过蓝图脚本进行读写操作，便于游戏开发中的灵活配置与使用。
 *
 * @note 确保为 `ConsumableType` 设置有效的枚举值，以避免运行时可能出现的错误。
 */
USTRUCT(BlueprintType, DisplayName="消耗品基础信息")
struct FLxConsumableCoreType
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="消耗品类型")
	ELxConsumableType ConsumableType;
};

