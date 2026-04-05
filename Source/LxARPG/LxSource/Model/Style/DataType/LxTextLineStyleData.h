// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxTextLineStyleData.generated.h"

USTRUCT(BlueprintType, DisplayName="单行文本样式")
struct FLxTextLineStyleData : public FTableRowBase
{
	GENERATED_BODY()

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="样式ID")
	FName ID;
	
	/**
	 * @var TSoftObjectPtr<UTexture2D> IconTexture
	 * @brief 文本行的图标
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本图标")
	TSoftObjectPtr<UTexture2D> IconTexture;

	// ===== 文本 =====

	/**
	 * @var FSlateFontInfo Font
	 * @brief 字体相关
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="字体相关")
	FSlateFontInfo Font;

	/**
	 * @var FSlateColor TextColor
	 * @brief 文字颜色
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文字颜色")
	FSlateColor TextColor;

	/**
	 * @var FLinearColor ShadowColor
	 * @brief 阴影颜色
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="阴影颜色")
	FLinearColor ShadowColor;

	/**
	 * @var FVector2D ShadowOffset
	 * @brief 阴影大小
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="阴影大小")
	FVector2D ShadowOffset;

	/**
	 * @var float LineHeightPercentage
	 * @brief 行高
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="行高")
	float LineHeightPercentage = 1.0f;

	/**
	 * @var TEnumAsByte<ETextJustify::Type> Justification
	 * @brief 对齐方式
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="对齐方式")
	TEnumAsByte<ETextJustify::Type> Justification;
};
