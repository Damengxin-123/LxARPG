// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxMaterialCoreType.h"
#include "LxMaterialEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "UObject/Object.h"
#include "LxMaterial.generated.h"

/**
 * @brief 材料物品定义类型
 *
 * 该结构体继承自`FLxItemDefineBase`，用于定义游戏中材料类物品的具体属性。它包含了一个`FLxMaterialCoreInfo`类型的成员变量，用以描述材料的基础属性。
 * 此外，通过继承自`FLxItemDefineBase`，`FLxMaterialDefine`还间接地包含了所有物品共有的基础信息、堆叠信息以及可视化信息等。
 * 该结构体支持在蓝图中被实例化，并且其属性可以通过编辑器进行设置和读写。
 */
USTRUCT(BlueprintType, DisplayName="材料物品定义类型")
struct FLxMaterialDefine : public FLxItemDefineBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="材料基础属性")
	FLxMaterialCoreInfo MaterialCoreInfo;
};

/**
 * @brief 材料物品缓存类型
 *
 * 该结构体继承自`FLxItemDateBase`，用于存储游戏中材料类物品的缓存数据。它包含了一个`FLxMaterialCoreInfo`类型的成员变量，用以描述材料的基础属性。
 * 此外，通过继承自`FLxItemDateBase`，`FLxMaterialData`还间接地包含了所有物品共有的基础信息、堆叠信息以及可视化信息等。
 * 该结构体支持在蓝图中被实例化，并且其属性可以通过编辑器进行设置和读写。
 */
USTRUCT(BlueprintType, DisplayName="材料物品缓存类型")
struct FLxMaterialData : public FLxItemDateBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="材料基础属性")
	FLxMaterialCoreInfo MaterialCoreInfo;
};