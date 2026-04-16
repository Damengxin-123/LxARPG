// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LxDataTableConfigBase.generated.h"

/**
 * @brief 数据表配置基类
 */
UCLASS(Blueprintable, DisplayName="数据表配置基类")
class LXARPG_API ULxDataTableConfigBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
 	 * @brief 初始化数据表加载
 	 *
 	 * 该虚函数用于初始化数据表的加载过程。子类可以重写此方法以实现特定的数据表加载逻辑。
 	 */
	virtual void InitDataTableLoading() {};
};
