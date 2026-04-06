// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuffCoreType.h"
#include "LxBuffEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "UObject/Object.h"
#include "LxBuff.generated.h"

/**
 * @brief Buff定义类型
 * 该结构体继承自FLxItemDefineBase，用于定义游戏中的Buff相关信息。它包含了一个核心信息字段BuffCoreInfo，该字段包含了Buff的类型等关键属性。
 */
USTRUCT(BlueprintType, DisplayName="Buff定义类型")
struct FBuffDefine : public FLxItemDefineBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff定义核心信息")
	FBuffCoreInfo BuffCoreInfo;
};

/**
 * @brief Buff缓存类型
 * 该结构体继承自FLxItemDateBase，用于在游戏中存储和管理Buff的相关数据。它包含了一个核心信息字段BuffCoreInfo，该字段包含了Buff的类型等关键属性。
 * 可以在编辑器中设置，并且可以通过蓝图读写。
 */
USTRUCT(BlueprintType, DisplayName="Buff缓存类型")
struct FBuffData : public FLxItemDateBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff定义核心信息")
	FBuffCoreInfo BuffCoreInfo;
};
