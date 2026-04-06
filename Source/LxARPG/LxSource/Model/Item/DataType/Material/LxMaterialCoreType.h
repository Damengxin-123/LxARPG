// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxMaterialEnum.h"
#include "UObject/Object.h"
#include "LxMaterialCoreType.generated.h"

USTRUCT(BlueprintType, DisplayName="材料基础信息")
struct FLxMaterialCoreInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="材料类型")
	ELxMaterialType MaterialType;
};

