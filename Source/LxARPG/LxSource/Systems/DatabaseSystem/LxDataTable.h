// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxDataTableTypeEnum.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxARPG/LxSource/Model/Input/LxInputData.h"
#include "StructUtils/InstancedStruct.h"
#include "LxDataTable.generated.h"

/**
 * @brief 数据表管理类型
 * 子类蓝图类通过继承此类型，并设置要加载的数据表格，无需在蓝图中调用任何函数，在ULxGameDataTablesManager中会自动调用数据表格加载函数
 * 注意，数据表格必须是同一类型
 */
UCLASS(Blueprintable, DisplayName="数据表格加载对象")
class LXARPG_API ULxDataTable : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	// 加载数据表到缓存
	void LoadDataTables(const ELxDataTableTypeEnum InTableType, const FString& InContextString);

	// 数据表列表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="数据管理|数据表列表", DisplayName="同一类型的数据表集合")
	TArray<UDataTable*> m_vDataTableList;
	
	// 加载表格到缓存表中
	template <class T>
	void LoadDataTable(const FString& ContextString);
	
	// 获取单个数据表行
	template <class T>
	const T* GetData(const FName& RowName) const;

	// 设置数据获取下标
	void SetIteratorIndex(uint16 InIndex = 0);

	/*
	 * 获取下一条数据 如果为nullptr则表明已经到头了
	 * 注：由于数据存储是map表，不具备有效的顺序存储，
	 * 所以此函数仅仅用于必要时遍历，当内部数据发生改变时，
	 * 顺序会发生改变，无法通过下标有效的获取具体 的某个值
	 */
	template <class T>
	const T* GetIteratorData() const;

private:

	uint16 m_nIteratorIndex = 0;
	
	// 缓存的数据表map表，在初始化完成之后，会将所有的表格都写入到此缓存中，便于查询
	UPROPERTY()
	TMap<FName, FInstancedStruct> m_mapTableRows;
	// 用于下标获取数据时map表的索引表
	UPROPERTY()
	TArray<FName> m_vTableRowKeys;
};

template <class T>
void ULxDataTable::LoadDataTable(const FString& ContextString)
{
	TArray<T*> Rows;
	// 加载数据表
	for (auto& tab : m_vDataTableList)
	{
		if (tab)
		{
			tab->GetAllRows<T>(ContextString, Rows);
		}
	}
	// 存入缓存
	for (auto& data : Rows)
	{
		if (data)
		{
			FInstancedStruct InstancedStruct;
			InstancedStruct.InitializeAs<T>(*data);
			m_mapTableRows.Add(data->RowID, InstancedStruct);
			m_vTableRowKeys.Add(data->RowID);
		}
	}
}


template <class T>
const T* ULxDataTable::GetData(const FName& RowName) const
{
	if (m_mapTableRows.Contains(RowName))
	{
		const FInstancedStruct& InstancedStruct = m_mapTableRows[RowName];
		if (T::StaticStruct() == InstancedStruct.GetScriptStruct())
		{
			return InstancedStruct.GetPtr<T>();
		}
	}
	return nullptr;
}

template <class T>
const T* ULxDataTable::GetIteratorData() const
{
	// 判断是否越界或为空
	if (m_vTableRowKeys.IsEmpty() || m_nIteratorIndex >= m_vTableRowKeys.Num())
	{
		return nullptr;
	}
	const FInstancedStruct& InstancedStruct = m_mapTableRows[m_vTableRowKeys[m_nIteratorIndex]];
	if (T::StaticStruct() == InstancedStruct.GetScriptStruct())
	{
		return InstancedStruct.GetPtr<T>();
	}
	return nullptr;
}
