// Fill out your copyright notice in the Description page of Project Settings.

#include "LxGameDataTablesManager.h"

#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Style/TableConfig/LxTextLineStyleDataConfig.h"
#include "InputCoreTypes.h"

namespace
{
	template<typename RowType, typename SetterType>
	void LoadEntryDataTable(const UDataTable* InDataTable, const TCHAR* InContextString, SetterType InSetter)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		TArray<RowType*> Rows;
		InDataTable->GetAllRows<RowType>(InContextString, Rows);

		for (const RowType* RowData : Rows)
		{
			if (RowData == nullptr)
			{
				continue;
			}

			InSetter(*RowData);
		}
	}

	/** 确保玩家瞄准输入有默认右键配置，数据表中已配置时保持数据表优先。 */
	void EnsureDefaultAimInputActionInfo()
	{
		if (LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Aim))
		{
			return;
		}

		FLxInputActionInfo AimInputActionInfo;
		AimInputActionInfo.InputActionID = ELxInputActionID::Aim;
		AimInputActionInfo.DisplayName = FText::FromString(TEXT("瞄准"));
		AimInputActionInfo.ValueType = EInputActionValueType::Boolean;
		AimInputActionInfo.InteractionType = ELxInputInteractionType::PressAndRelease;
		AimInputActionInfo.DefaultKey = EKeys::RightMouseButton;
		AimInputActionInfo.ValueDirection = ELxInputValueAxial::None;
		AimInputActionInfo.ValueMagnification = 1.f;

		LxInputActionConfig::SetInputActionInfo(AimInputActionInfo);
	}

	void LoadCharacterAttributeDataTable(const UDataTable* InDataTable)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		TArray<FLxAttributeData*> Rows;
		InDataTable->GetAllRows<FLxAttributeData>(TEXT("ULxGameDataTablesManager::LoadCharacterAttributeDataTable"), Rows);

		for (const FLxAttributeData* RowData : Rows)
		{
			if (RowData == nullptr || RowData->AttributeID == ELxCharacterAttributeID::X_None)
			{
				continue;
			}

			LxAttributeConfig::SetAttributeDataConfig(*RowData);
		}
	}

	void LoadRaceAttributeValueConfigTable(ELxCharacterRaceType InRaceType, const UDataTable* InDataTable)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		TArray<FLxAttributeValueConfig*> Rows;
		InDataTable->GetAllRows<FLxAttributeValueConfig>(TEXT("ULxGameDataTablesManager::LoadRaceAttributeValueConfigTable"), Rows);

		TArray<FLxAttributeValueConfig> ValueConfigList;
		for (const FLxAttributeValueConfig* RowData : Rows)
		{
			if (RowData == nullptr || RowData->AttributeID == ELxCharacterAttributeID::X_None)
			{
				continue;
			}

			ValueConfigList.Add(*RowData);
		}

		LxAttributeConfig::SetCharacterRaceBaseAttributeValues(InRaceType, ValueConfigList);
	}

	void LoadInputActionInfoDataTable(const UDataTable* InDataTable)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		TArray<FLxInputActionInfo*> Rows;
		InDataTable->GetAllRows<FLxInputActionInfo>(TEXT("ULxGameDataTablesManager::LoadInputActionInfoDataTable"), Rows);

		for (const FLxInputActionInfo* RowData : Rows)
		{
			if (RowData == nullptr || RowData->InputActionID == ELxInputActionID::None)
			{
				continue;
			}

			LxInputActionConfig::SetInputActionInfo(*RowData);
		}

		EnsureDefaultAimInputActionInfo();
	}

	template<typename RowType, typename SetterType>
	void LoadItemDataTable(const UDataTable* InDataTable, const TCHAR* InContextString, SetterType InSetter)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		TArray<RowType*> Rows;
		InDataTable->GetAllRows<RowType>(InContextString, Rows);

		for (const RowType* RowData : Rows)
		{
			if (RowData == nullptr || !RowData->ItemIDTag.IsValid())
			{
				continue;
			}

			InSetter(*RowData);
		}
	}
}


void ULxGameDataTablesManager::LoadDataTables()
{

	LxAttributeConfig::ClearAttributeConfig();
	LxEntryConfig::ClearEntryConfig();
	LxInputActionConfig::ClearInputActionConfig();
	LxItemConfig::ClearItemConfig();

	LoadInputActionInfoDataTable(m_pInputActionInfoTableConfig.Get());
	LoadCharacterAttributeDataTable(m_pCharacterAttributeDataTable.Get());

	for (const TPair<ELxCharacterRaceType, TObjectPtr<UDataTable>>& RaceTablePair : m_mapRaceAttributeValueConfigTables)
	{
		LoadRaceAttributeValueConfigTable(RaceTablePair.Key, RaceTablePair.Value.Get());
	}

	LoadEntryDataTable<FLxEntryAttributeGain>(
		m_pAttributeGainEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeGainEntryTable"),
		[](const FLxEntryAttributeGain& RowData)
		{
			LxEntryConfig::SetAttributeGainEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryAttributeRecovery>(
		m_pAttributeRecoveryEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeRecoveryEntryTable"),
		[](const FLxEntryAttributeRecovery& RowData)
		{
			LxEntryConfig::SetAttributeRecoveryEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryChangeState>(
		m_pChangeStateEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadChangeStateEntryTable"),
		[](const FLxEntryChangeState& RowData)
		{
			LxEntryConfig::SetChangeStateEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryCreateBuff>(
		m_pCreateBuffEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadCreateBuffEntryTable"),
		[](const FLxEntryCreateBuff& RowData)
		{
			LxEntryConfig::SetCreateBuffEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryMultiTarget>(
		m_pMultiTargetEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadMultiTargetEntryTable"),
		[](const FLxEntryMultiTarget& RowData)
		{
			LxEntryConfig::SetMultiTargetEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryDisplayText>(
		m_pDisplayTextEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadDisplayTextEntryTable"),
		[](const FLxEntryDisplayText& RowData)
		{
			LxEntryConfig::SetDisplayTextEntryData(RowData);
		});

	LoadItemDataTable<FLxEquipmentInformation>(
		m_pEquipmentItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadEquipmentItemTable"),
		[](const FLxEquipmentInformation& RowData)
		{
			LxItemConfig::SetEquipmentItemData(RowData);
		});

	LoadItemDataTable<FLxConsumableInformation>(
		m_pConsumableItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadConsumableItemTable"),
		[](const FLxConsumableInformation& RowData)
		{
			LxItemConfig::SetConsumableItemData(RowData);
		});

	LoadItemDataTable<FLxMaterialInformation>(
		m_pMaterialItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadMaterialItemTable"),
		[](const FLxMaterialInformation& RowData)
		{
			LxItemConfig::SetMaterialItemData(RowData);
		});

	LoadItemDataTable<FLxBuffInformation>(
		m_pBuffItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadBuffItemTable"),
		[](const FLxBuffInformation& RowData)
		{
			LxItemConfig::SetBuffItemData(RowData);
		});

	LoadItemDataTable<FLxSkillItemInformation>(
		m_pSkillItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadSkillItemTable"),
		[](const FLxSkillItemInformation& RowData)
		{
			LxItemConfig::SetSkillItemData(RowData);
		});
}
