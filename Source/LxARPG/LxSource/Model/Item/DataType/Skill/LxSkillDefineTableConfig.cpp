// Fill out your copyright notice in the Description page of Project Settings.

#include "LxSkillDefineTableConfig.h"

void ULxSkillDefineTableConfig::InitDataTableLoading()
{
	m_tSkillDefineMap.Empty();

	for (UDataTable* Table : m_vSkillDefineTableList)
	{
		if (Table == nullptr)
		{
			continue;
		}

		TArray<FLxSkillDefine*> Rows;
		Table->GetAllRows<FLxSkillDefine>(TEXT("ULxSkillDefineTableConfig"), Rows);

		for (const FLxSkillDefine* RowData : Rows)
		{
			if (RowData == nullptr || RowData->ItemInfo.ItemID.IsNone())
			{
				continue;
			}

			m_tSkillDefineMap.Add(RowData->ItemInfo.ItemID, *RowData);
		}
	}
}

const FLxSkillDefine* ULxSkillDefineTableConfig::GetSkillDefine(const FName& InItemID) const
{
	if (InItemID.IsNone())
	{
		return nullptr;
	}

	return m_tSkillDefineMap.Find(InItemID);
}
