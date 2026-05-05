// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxItemRarityType.generated.h"


/**
 * @brief 物品稀有度枚举类型
 *
 * 该枚举用于表示物品的稀有程度，从“错误稀有度”到“神话”。
 * 枚举值可以通过蓝图访问，并且在编辑器中显示为中文名称。
 */
UENUM(BlueprintType, DisplayName="物品稀有度")
enum class ELxItemRarityType : uint8
{
	// 错误稀有度
	None		UMETA(DisplayName = "错误稀有度"),
	// 普通
	Ordinary	UMETA(DisplayName = "普通"),
	// 高级
	Advanced	UMETA(DisplayName = "高级"),
	// 稀有
	Rare		UMETA(DisplayName = "稀有"),
	// 传说
	Legend		UMETA(DisplayName = "传说"),
	// 史诗
	Epic		UMETA(DisplayName = "史诗"),
	// 神话
	Mythic		UMETA(DisplayName = "神话"),
};










