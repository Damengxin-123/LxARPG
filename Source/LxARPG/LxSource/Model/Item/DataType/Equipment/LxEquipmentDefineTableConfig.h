// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxEquipment.h"
#include "LxEquipmentDefineTableConfig.generated.h"

class UDataTable;

/**
 * @brief 装备定义数据表配置
 *
 * 支持配置多张 FLxEquipmentDefine 类型数据表，并在初始化时缓存为 ItemID 索引。
 */
UCLASS(Blueprintable, DisplayName="装备定义表格配置")
class LXARPG_API ULxEquipmentDefineTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxEquipmentDefine* GetEquipmentDefine(const FName& InItemID) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备定义配置表列表")
	TArray<UDataTable*> m_vEquipmentDefineTableList;

private:
	TMap<FName, FLxEquipmentDefine> m_tEquipmentDefineMap;
};

