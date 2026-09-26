// Fill out your copyright notice in the Description page of Project Settings.

#include "LxGameDataTablesManager.h"

#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTableConfig.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleConfig.h"
#include "InputCoreTypes.h"

namespace
{
	/** 统一遍历数据表；具体行的有效性由各配置写入接口检查。 */
	template<typename RowType, typename SetterType>
	void LoadConfigDataTable(const UDataTable* InDataTable, const TCHAR* InContextString, SetterType InSetter)
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

	/** 加载富文本样式映射，并保留样式表类型检查。 */
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

}

void ULxGameDataTablesManager::LoadDataTables()
{

	LxEntryConfig::ClearEntryConfig();
	LxInputActionConfig::ClearInputActionConfig();
	LxItemConfig::ClearItemConfig();
	LxProfessionConfig::ClearProfessionConfig();
	LxRichTextStyleConfig::ClearRichTextStyleConfig();

	LoadConfigDataTable<FLxInputActionInfo>(
		m_pInputActionInfoTableConfig.Get(),
		TEXT("ULxGameDataTablesManager::LoadInputActionInfoDataTable"),
		LxInputActionConfig::SetInputActionInfo);
	EnsureDefaultInputActionInfos();
	LoadConfigDataTable<FLxProfessionDefinitionTableRow>(
		m_pProfessionDefinitionTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadProfessionDefinitionDataTable"),
		LxProfessionConfig::SetProfessionDefinitionTableRow);
	LoadRichTextStyleMappingDataTable(m_pRichTextStyleTable.Get());

	LoadConfigDataTable<FLxEntryAttributeGain>(
		m_pAttributeGainEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeGainEntryTable"),
		LxEntryConfig::SetAttributeGainEntryData);

	LoadConfigDataTable<FLxEntryAttributeInfluence>(
		m_pAttributeInfluenceEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeInfluenceEntryTable"),
		LxEntryConfig::SetAttributeInfluenceEntryData);

	LoadConfigDataTable<FLxEntryAttributeRecovery>(
		m_pAttributeRecoveryEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadAttributeRecoveryEntryTable"),
		LxEntryConfig::SetAttributeRecoveryEntryData);

	LoadConfigDataTable<FLxEntryChangeState>(
		m_pChangeStateEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadChangeStateEntryTable"),
		LxEntryConfig::SetChangeStateEntryData);

	LoadConfigDataTable<FLxEntryCreateBuff>(
		m_pCreateBuffEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadCreateBuffEntryTable"),
		LxEntryConfig::SetCreateBuffEntryData);

	LoadConfigDataTable<FLxEntryMultiTarget>(
		m_pMultiTargetEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadMultiTargetEntryTable"),
		LxEntryConfig::SetMultiTargetEntryData);

	LoadConfigDataTable<FLxEntryDisplayText>(
		m_pDisplayTextEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadDisplayTextEntryTable"),
		LxEntryConfig::SetDisplayTextEntryData);

	LoadConfigDataTable<FLxEntryGrantSkill>(
		m_pGrantSkillEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadGrantSkillEntryTable"),
		LxEntryConfig::SetGrantSkillEntryData);

	LoadConfigDataTable<FLxEntryGrantProfession>(
		m_pGrantProfessionEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadGrantProfessionEntryTable"),
		LxEntryConfig::SetGrantProfessionEntryData);

	LoadConfigDataTable<FLxEntryDamage>(
		m_pDamageEntryTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadDamageEntryTable"),
		LxEntryConfig::SetDamageEntryData);

	LoadConfigDataTable<FLxEquipmentInformation>(
		m_pEquipmentItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadEquipmentItemTable"),
		LxItemConfig::SetEquipmentItemData);

	LoadConfigDataTable<FLxConsumableInformation>(
		m_pConsumableItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadConsumableItemTable"),
		LxItemConfig::SetConsumableItemData);

	LoadConfigDataTable<FLxMaterialInformation>(
		m_pMaterialItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadMaterialItemTable"),
		LxItemConfig::SetMaterialItemData);

	LoadConfigDataTable<FLxBuffInformation>(
		m_pBuffItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadBuffItemTable"),
		LxItemConfig::SetBuffItemData);

	LoadConfigDataTable<FLxSkillItemInformation>(
		m_pSkillItemTable.Get(),
		TEXT("ULxGameDataTablesManager::LoadSkillItemTable"),
		LxItemConfig::SetSkillItemData);
}
