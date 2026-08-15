// Fill out your copyright notice in the Description page of Project Settings.

#include "LxGameDataTablesManager.h"

#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTableConfig.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleConfig.h"
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

	/** 确保职业界面快捷键拥有默认 P 键配置，数据表中已配置时保持数据表优先。 */
	void EnsureDefaultProfessionInputActionInfo()
	{
		if (LxInputActionConfig::GetInputActionInfo(ELxInputActionID::Profession))
		{
			return;
		}

		FLxInputActionInfo ProfessionInputActionInfo;
		ProfessionInputActionInfo.InputActionID = ELxInputActionID::Profession;
		ProfessionInputActionInfo.DisplayName = FText::FromString(TEXT("职业界面"));
		ProfessionInputActionInfo.ValueType = EInputActionValueType::Boolean;
		ProfessionInputActionInfo.InteractionType = ELxInputInteractionType::SingleTrigger;
		ProfessionInputActionInfo.DefaultKey = EKeys::P;
		ProfessionInputActionInfo.ValueDirection = ELxInputValueAxial::None;
		ProfessionInputActionInfo.ValueMagnification = 1.f;

		LxInputActionConfig::SetInputActionInfo(ProfessionInputActionInfo);
	}

	/** 确保关键默认输入行为存在，避免新增功能必须同步修改数据表才能使用。 */
	void EnsureDefaultInputActionInfos()
	{
		EnsureDefaultAimInputActionInfo();
		EnsureDefaultProfessionInputActionInfo();
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

		EnsureDefaultInputActionInfos();
	}

	void LoadProfessionDefinitionDataTable(const UDataTable* InDataTable)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		TArray<FLxProfessionDefinitionTableRow*> Rows;
		InDataTable->GetAllRows<FLxProfessionDefinitionTableRow>(TEXT("ULxGameDataTablesManager::LoadProfessionDefinitionDataTable"), Rows);

		for (const FLxProfessionDefinitionTableRow* RowData : Rows)
		{
			if (RowData == nullptr || !RowData->ProfessionIDTag.IsValid() || !RowData->ProfessionClass)
			{
				continue;
			}

			LxProfessionConfig::SetProfessionDefinitionTableRow(*RowData);
		}
	}

	void LoadRichTextStyleMappingDataTable(UDataTable* InDataTable)
	{
		if (InDataTable == nullptr)
		{
			return;
		}

		const UScriptStruct* RowStruct = InDataTable->GetRowStruct();
		if (RowStruct == nullptr || !RowStruct->IsChildOf(FLxRichTextStyleRow::StaticStruct()))
		{
			return;
		}

		LxRichTextStyleConfig::SetRichTextStyleDataTable(InDataTable);
		for (const TPair<FName, uint8*>& RowPair : InDataTable->GetRowMap())
		{
			const FLxRichTextStyleRow* StyleRow = reinterpret_cast<const FLxRichTextStyleRow*>(RowPair.Value);
			if (StyleRow == nullptr)
			{
				continue;
			}

			LxRichTextStyleConfig::SetRichTextStyleRow(RowPair.Key, *StyleRow);
		}
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

	LxEntryConfig::ClearEntryConfig();
	LxInputActionConfig::ClearInputActionConfig();
	LxItemConfig::ClearItemConfig();
	LxProfessionConfig::ClearProfessionConfig();
	LxRichTextStyleConfig::ClearRichTextStyleConfig();

	LoadInputActionInfoDataTable(m_pInputActionInfoTableConfig.Get());
	EnsureDefaultInputActionInfos();
	LoadProfessionDefinitionDataTable(m_pProfessionDefinitionTable.Get());
	LoadRichTextStyleMappingDataTable(m_pRichTextStyleTable.Get());

	LoadEntryDataTable<FLxEntryAttributeGain>(
		m_pAttributeGainEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeGainEntryTable"),
		[](const FLxEntryAttributeGain& RowData)
		{
			LxEntryConfig::SetAttributeGainEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryAttributeInfluence>(
		m_pAttributeInfluenceEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeInfluenceEntryTable"),
		[](const FLxEntryAttributeInfluence& RowData)
		{
			LxEntryConfig::SetAttributeInfluenceEntryData(RowData);
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

	LoadEntryDataTable<FLxEntryGrantSkill>(
		m_pGrantSkillEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadGrantSkillEntryTable"),
		[](const FLxEntryGrantSkill& RowData)
		{
			LxEntryConfig::SetGrantSkillEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryGrantProfession>(
		m_pGrantProfessionEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadGrantProfessionEntryTable"),
		[](const FLxEntryGrantProfession& RowData)
		{
			LxEntryConfig::SetGrantProfessionEntryData(RowData);
		});

	LoadEntryDataTable<FLxEntryDamage>(
		m_pDamageEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadDamageEntryTable"),
		[](const FLxEntryDamage& RowData)
		{
			LxEntryConfig::SetDamageEntryData(RowData);
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
