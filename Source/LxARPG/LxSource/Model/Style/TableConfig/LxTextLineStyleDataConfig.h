// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../DataType/LxTextLineStyleData.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxTextLineStyleDataConfig.generated.h"

/**
 * @brief 单行文本样式数据表配置类
 */
UCLASS()
class LXARPG_API ULxTextLineStyleDataConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()
public:
	virtual void InitDataTableLoading() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="样式表配置")
	TArray<UDataTable*> m_vTableList;
private:
	/**
 	 * @brief 存储文本行样式数据的映射表
 	 *
 	 * 该映射表使用FName作为键，FLxTextLineStyleData作为值，用于存储和检索不同名称对应的文本行样式数据。
 	 */
	TMap<FName, FLxTextLineStyleData>  m_tStyleDataMap;
};
