// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuffCoreType.h"
#include "LxBuffEnum.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "UObject/Object.h"
#include "LxBuff.generated.h"

class ULxItemEntryLogic;


USTRUCT(BlueprintType, DisplayName="Buff定义类型")
struct FLxBuffDefine : public FLxItemDefineBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff基础信息")
	FLxBuffCoreInfo BuffCoreInfo;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff效果信息")
    FLxBuffEffectInfo BuffEffectInfo;

	// buff引用列表中，词条比例默认为1，且不设置词条比例，词条比例由创建buff的词条传递
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff词条列表")
	TArray<FLxItemEntryQuote> BuffEntryList;

	FLxBuffDefine()
	{
		ItemInfo.ItemType = ELxItemType::Buff;		// 将物品类型设置为Buff
		ItemStackInfo.ItemCanStack = false;			// Buff通常不能堆叠
		ItemStackInfo.ItemMaxCount = 1;				// 最大堆叠数量
	}
};

USTRUCT(BlueprintType, DisplayName="Buff缓存类型")
struct FLxBuffData : public FLxItemDateBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff核心信息")
	FLxBuffCoreInfo BuffCoreInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff效果信息")
	FLxBuffEffectInfo BuffEffectInfo;

	// buff引用列表中，词条比例默认为1，且不设置词条比例，词条比例由创建buff的词条传递
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff词条列表")
	TArray<TObjectPtr<ULxItemEntryLogic>> BuffEntryList;

	FLxBuffData()
	{
		ItemInfo.ItemType = ELxItemType::Buff;		// 将物品类型设置为Buff
		ItemStackInfo.ItemCanStack = false;			// Buff通常不能堆叠
		ItemStackInfo.ItemMaxCount = 1;				// 最大堆叠数量
	}
};
