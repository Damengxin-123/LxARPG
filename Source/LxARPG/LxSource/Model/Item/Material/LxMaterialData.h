// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxMaterialEnum.h"
#include "LxARPG/LxSource/Model/Item/ItemBase/LxItemDataType.h"
#include "UObject/Object.h"
#include "LxMaterialData.generated.h"

/**
 * @struct FLxMaterialData
 * @brief 材料属性
 *
 * 继承自FLxItemAttributes，用于定义材料类型的属性。
 *
 * @note 可在蓝图中使用
 */
USTRUCT(BlueprintType)
struct FLxMaterialData : public FLxItemBase
{
	GENERATED_BODY()

	/**
	 * @var ELxMaterialType MaterialType
	 * @brief 材料类型
	 */
	ELxMaterialType MaterialType;

	virtual void InitData() override{};

};