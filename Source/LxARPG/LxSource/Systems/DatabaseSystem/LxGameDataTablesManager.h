// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxDataTableTypeEnum.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxGameDataTablesManager.generated.h"

class ULxDataTableConfigBase;
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
	ULxDataTableConfigBase* GetDataTables(const ELxDataTableTypeEnum InDataTableType) const;

	/**
	 * @brief 判断指定类型的数据表是否已加载完成。
	 *
	 * @param InDataTableType 要检查的数据表类型。
	 * @return 已加载完成返回 true，否则返回 false。
	 */
	bool TableIsLoadingCompleted(const ELxDataTableTypeEnum InDataTableType) const;

	UPROPERTY(EditDefaultsOnly, Category="数据表|数据表类型map", DisplayName="数据表类型设置")
	TMap<ELxDataTableTypeEnum, TSubclassOf<ULxDataTableConfigBase>> m_mapTablesSetting;

	/**
	 * @brief 加载全部配置的数据表对象。
	 *
	 * 会根据类型配置实例化并缓存各类数据表加载对象。
	 */
	virtual void LoadDataTables() override;

private:
	UPROPERTY()
	TMap<ELxDataTableTypeEnum, TObjectPtr<ULxDataTableConfigBase>> m_mapTablesMap;
};
