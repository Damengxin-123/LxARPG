// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LxDataTableBase.generated.h"

/**
 * 数据表管理类型基础类型
 */
UCLASS()
class LXARPG_API ULxDataTableBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	virtual void LoadDataTables(){};


};

