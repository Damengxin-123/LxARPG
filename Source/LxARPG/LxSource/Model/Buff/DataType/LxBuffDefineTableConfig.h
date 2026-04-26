// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxBuff.h"
#include "LxBuffDefineTableConfig.generated.h"

class UDataTable;

/**
 * @class ULxBuffDefineTableConfig
 * @brief 用于管理Buff定义数据表的配置类。
 *
 * 继承自ULxDataTableConfigBase，专门用于加载和提供对Buff定义数据表的访问。此类通过维护一个内部的数据映射来存储从数据表中读取的Buff定义信息，
 * 并对外提供了获取特定Buff定义的方法。
 */
UCLASS(Blueprintable, DisplayName="Buff定义表格配置")
class LXARPG_API ULxBuffDefineTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxBuffDefine* GetBuffDefine(const FName& InItemID) const;
	const FLxBuffDefine* GetBuffDefineByBuffID(ELxBuffID InBuffID) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Buff Define Table List")
	TArray<UDataTable*> m_vBuffDefineTableList;

private:
	TMap<FName, FLxBuffDefine> m_tBuffDefineMap;
	TMap<ELxBuffID, FLxBuffDefine> m_tBuffIDDefineMap;
};
