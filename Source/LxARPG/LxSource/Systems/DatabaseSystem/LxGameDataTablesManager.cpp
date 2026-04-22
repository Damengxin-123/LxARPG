// Fill out your copyright notice in the Description page of Project Settings.

#include "LxGameDataTablesManager.h"

#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionInfoTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Buff/LxBuffDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterialDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillDefineTableConfig.h"
#include "LxARPG/LxSource/Model/Style/TableConfig/LxTextLineStyleDataConfig.h"


void ULxGameDataTablesManager::LoadDataTables()
{
	ULxDataTableConfigBase* ConfigList[] =
	{
		m_pInputActionInfoTableConfig.Get(),
		m_pCharacterAttributeTableConfig.Get(),
		m_pTextLineStyleDataConfig.Get(),
		m_pItemEntryDefineTableConfig.Get(),
		m_pEquipmentDefineTableConfig.Get(),
		m_pConsumableDefineTableConfig.Get(),
		m_pMaterialDefineTableConfig.Get(),
		m_pSkillDefineTableConfig.Get(),
		m_pBuffDefineTableConfig.Get(),
	};

	for (ULxDataTableConfigBase* Config : ConfigList)
	{
		if (Config)
		{
			Config->InitDataTableLoading();
		}
	}
}
