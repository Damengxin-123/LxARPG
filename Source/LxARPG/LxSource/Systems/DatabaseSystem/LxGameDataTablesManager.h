// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxDataTableTypeEnum.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxGameDataTablesManager.generated.h"

class ULxDataTable;
/**
 * 加载本地数据表格 总表
 */
UCLASS(Blueprintable, DisplayName="数据表格管理对象")
class LXARPG_API ULxGameDataTablesManager : public ULxDataTableBase
{
	GENERATED_BODY()

public:
	/**
	 * 获取指定类型的数据表
	 *
	 * @param InDataTableType 数据表类型枚举
	 * @return 返回对应类型的ULxDataTable对象，如果未找到则返回nullptr
	 */
	ULxDataTable* GetDataTables(const ELxDataTableTypeEnum InDataTableType) const;

	bool TableIsLoadingCompleted(const ELxDataTableTypeEnum InDataTableType) const;

	UPROPERTY(EditDefaultsOnly, Category="数据表|数据表类型map", DisplayName="数据表类型设置")
	TMap<ELxDataTableTypeEnum, TSubclassOf<ULxDataTable>> m_mapTablesSetting;
	
	
	// 输入行为数据表，具体定义了用户输入行为的相关信息的表 的 管理对象 
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="数据管理|输入行为数据表", DisplayName="输入行为数据表")
	// TObjectPtr<ULxDataTable> InputDataBase;

	

	virtual void LoadDataTables() override;

private:
	UPROPERTY()
	TMap<ELxDataTableTypeEnum, TObjectPtr<ULxDataTable>> m_mapTablesMap;
};
