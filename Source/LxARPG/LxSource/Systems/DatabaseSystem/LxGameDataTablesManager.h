// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxGameDataTablesManager.generated.h"

class ULxDataTableConfigBase;
class ULxAttributeTableConfig;
class ULxTextLineStyleDataConfig;
class ULxInputActionInfoTableConfig;
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="输入信息表配置对象")
	TObjectPtr<ULxInputActionInfoTableConfig> m_pInputActionInfoTableConfig = nullptr;

	// 属性配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="属性配置表")
	TObjectPtr<ULxAttributeTableConfig> m_pCharacterAttributeTableConfig = nullptr;

	// 文本行样式配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="文本行样式配置表")
	TObjectPtr<ULxTextLineStyleDataConfig> m_pTextLineStyleDataConfig = nullptr;

	// 词条信息配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="词条定义配置表")
	TObjectPtr<ULxItemEntryDefineTableConfig> m_pItemEntryDefineTableConfig = nullptr;
	// 装备信息配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="装备定义配置表")
	TObjectPtr<ULxEquipmentDefineTableConfig> m_pEquipmentDefineTableConfig = nullptr;
	// 消耗品信息配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="消耗品定义配置表")
	TObjectPtr<ULxConsumableDefineTableConfig> m_pConsumableDefineTableConfig = nullptr;
	// 材料信息配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="材料定义配置表")
	TObjectPtr<ULxMaterialDefineTableConfig> m_pMaterialDefineTableConfig = nullptr;
	// 技能信息配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="技能定义配置表")
	TObjectPtr<ULxSkillDefineTableConfig> m_pSkillDefineTableConfig = nullptr;
	// Buff信息配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="Buff定义配置表")
	TObjectPtr<ULxBuffDefineTableConfig> m_pBuffDefineTableConfig = nullptr;

	virtual void LoadDataTables();
};
