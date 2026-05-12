// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemRarityType.h"
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
struct FLxRarityInfo : public FTableRowBase
{
	GENERATED_BODY()


	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "稀有度可视化", DisplayName="稀有度枚举值")
	ELxItemRarityType RarityType =  ELxItemRarityType::None;
	
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

	/**
	 * @var TSoftObjectPtr<UTexture2D> RarityBackgroundImage
	 * @brief 稀有度背景图标
	 *
	 * 用于在UI中显示表示该稀有度级别的背景图像。
	 * 通过软引用（TSoftObjectPtr）来引用UTexture2D类型的纹理资源，允许延迟加载和更灵活的资源管理。
	 *
	 * @note
	 * - 该属性可以在编辑器中进行设置，并且支持蓝图读写访问。
	 * - 显示名称为"稀有度图标"，归类于"稀有度可视化"类别下，便于在编辑器中查找和配置。
	 * - 使用软引用可以减少内存占用，提高加载效率，特别适用于需要频繁切换或加载大量不同稀有度图标的场景。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "稀有度可视化", DisplayName="稀有度图标")
	TSoftObjectPtr<UTexture2D> RarityBackgroundImage;
};
