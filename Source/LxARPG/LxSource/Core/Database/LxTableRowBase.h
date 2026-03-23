// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxTableRowBase.generated.h"


/**
 * @struct FLxTableRowBase
 * @brief 基础结构体数据
 *
 * 作为所有数据表结构体的基类，提供了数据表主键ID和基本的初始化、更新功能。
 * 继承自FTableRowBase，可在数据表中使用。
 *
 * @note 可在蓝图中使用，DisplayName为"基础结构体数据"
 */
USTRUCT(BlueprintType, DisplayName="基础结构体数据")
struct FLxTableRowBase : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * @var FName ID
	 * @brief 数据表主键
	 *
	 * 用于唯一标识数据表中的每一行数据。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="A数据表信息|主键", DisplayName="数据表主键")
	FName RowID = TEXT("null");

	/**
	 * @brief 初始化数据
	 *
	 * 在结构体创建时调用，用于初始化某些值。
	 * 例如：当使用数据表值填充此结构体后，需要从引用的表行中读取数据等。
	 */
	virtual void InitData(){};

	/**
	 * @brief 更新数据
	 *
	 * 更新数据表内的数据，用于在重新设置了某些值之后，对其他的值进行重新计算。
	 */
	virtual void UpdateData(){};


};

