// Fill out your copyright notice in the Description page of Project Settings.


#include "LxTextLineStyleDataConfig.h"

void ULxTextLineStyleDataConfig::InitDataTableLoading()
{
	TArray<FLxTextLineStyleData*> Rows;
	// 加载数据表
	for (auto& table : m_vTableList)
	{
		if (table)
		{
			table->GetAllRows<FLxTextLineStyleData>("ULxTextLineStyleDataConfig", Rows);
		}
	}
	// 存入缓存
	for (auto& data : Rows)
	{
		if (data)
		{
			m_tStyleDataMap.Add(data->ID, *data);
		}
	}
}
