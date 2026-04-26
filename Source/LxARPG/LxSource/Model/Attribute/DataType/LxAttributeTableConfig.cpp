// Fill out your copyright notice in the Description page of Project Settings.


#include "LxAttributeTableConfig.h"

#include "LxAttributeData.h"
#include "LxARPG/LxSource/Core/Tools/LxRichTextDescriptionTool.h"

void ULxAttributeTableConfig::InitDataTableLoading()
{
	m_tAttributeDataMap.Empty();

	// 遍历属性定义表格列表，加载每个表格中的属性定义值，并将其转换为属性数据存储在属性数据映射表中
	for (const auto& Pair : m_mapAttributeTableList)
	{
		if (Pair.Value == nullptr)
		{
			continue;
		}

		TArray<FLxAttributeDefineValue*> Rows;
		Pair.Value->GetAllRows<FLxAttributeDefineValue>(TEXT("ULxAttributeTableConfig"), Rows);

		TArray<FLxAttributeData>& AttributeDataList = m_tAttributeDataMap.FindOrAdd(Pair.Key);
		AttributeDataList.Reserve(Rows.Num());
		for (const FLxAttributeDefineValue* DefineValue : Rows)
		{
			if (DefineValue == nullptr)
			{
				continue;
			}

			const FLxAttributeDefineInfo* DefineInfo = DefineValue->AttributeTableQuote.GetRow<FLxAttributeDefineInfo>(TEXT("ULxAttributeTableConfig"));
			if (DefineInfo == nullptr)
			{
				continue;
			}
			
			FLxAttributeData AttributeData;
			AttributeData.DerivedRules = DefineInfo->DerivedRules;
			AttributeData.AttributeInfo = DefineInfo->AttributeInfo;
			AttributeData.AttributeShowInfo = DefineInfo->AttributeShowInfo;
			AttributeData.AttributeValue = DefineValue->AttributeValue;
			AttributeData.CalculatedAttributeValue = DefineValue->AttributeValue;
			
			AttributeDataList.Add(AttributeData);
		}
	}
}

const TArray<FLxAttributeData>* ULxAttributeTableConfig::GetAttributeDataList(ELxCharacterRaceType InRaceType) const
{
	if (m_tAttributeDataMap.Contains(InRaceType))
	{
		return &m_tAttributeDataMap[InRaceType];
	}
	return nullptr;
}
