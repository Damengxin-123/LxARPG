// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxTableRowBase.h"
#include "LxRarityInfoData.generated.h"


/**
 * @struct FLxRarityInfo
 * @brief 表示装备稀有度描述信息的结构体
 *
 * 继承自FTableRowBase，用于存储与装备稀有度相关的信息。
 *
 * @note
 * - 通过继承自FTableRowBase，可以利用基础结构体数据提供的功能
 * - 该结构体可以在蓝图中使用，并且其显示名称为"装备稀有度描述信息"
 * - 此结构体用于在蓝图中创建数据表，无需在代码中加载，在使用时直接通过表行信息全部获取即可
 */
USTRUCT(BlueprintType, DisplayName="装备稀有度描述信息")
struct FLxRarityInfo : public FLxTableRowBase
{
	GENERATED_BODY()

	/**
	 * @var uint8 RarityValue
	 * @brief 稀有度数值
	 *
	 * 用于在代码中进行比较，数值越大稀有度越高。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "稀有度属性", DisplayName="稀有度数值")
	uint8 RarityValue = 0;

	/**
	 * @var FText RarityName
	 * @brief 稀有度描述
	 *
	 * 用于在UI中显示的稀有度描述文本。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "稀有度可视化", DisplayName="稀有度描述")
	FText RarityText;

	/**
	 * @var FLinearColor RarityColor
	 * @brief 稀有度颜色
	 *
	 * 用于在UI中显示此稀有度的颜色。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "稀有度可视化", DisplayName="稀有度颜色")
	FLinearColor RarityColor;
};