// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxGameDataTablesManager.generated.h"

class ULxDataTableConfigBase;
class ULxAttributeTableConfig;
class ULxTextLineStyleDataConfig;
class ULxItemEntryDefineTableConfig;
class ULxEquipmentDefineTableConfig;
class ULxConsumableDefineTableConfig;
class ULxMaterialDefineTableConfig;
class ULxSkillDefineTableConfig;
class ULxBuffDefineTableConfig;

UCLASS(Blueprintable, DisplayName="数据表格管理对象")
class LXARPG_API ULxGameDataTablesManager : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Legacy table-config slots that are consumed directly by modules.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Input Action Info Config")
	TObjectPtr<ULxDataTableConfigBase> m_pInputActionInfoTableConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Character Attribute Config")
	TObjectPtr<ULxAttributeTableConfig> m_pCharacterAttributeTableConfig = nullptr;

	// Per-derived ULxDataTableConfigBase config slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Text Line Style Config")
	TObjectPtr<ULxTextLineStyleDataConfig> m_pTextLineStyleDataConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Item Entry Define Config")
	TObjectPtr<ULxItemEntryDefineTableConfig> m_pItemEntryDefineTableConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Equipment Define Config")
	TObjectPtr<ULxEquipmentDefineTableConfig> m_pEquipmentDefineTableConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Consumable Define Config")
	TObjectPtr<ULxConsumableDefineTableConfig> m_pConsumableDefineTableConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Material Define Config")
	TObjectPtr<ULxMaterialDefineTableConfig> m_pMaterialDefineTableConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Skill Define Config")
	TObjectPtr<ULxSkillDefineTableConfig> m_pSkillDefineTableConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Buff Define Config")
	TObjectPtr<ULxBuffDefineTableConfig> m_pBuffDefineTableConfig = nullptr;

	virtual void LoadDataTables();
};
