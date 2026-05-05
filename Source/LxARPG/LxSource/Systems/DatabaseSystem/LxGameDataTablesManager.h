// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxGameDataTablesManager.generated.h"

class UDataTable;
class ULxDataTableConfigBase;
class ULxTextLineStyleDataConfig;
class ULxInputActionInfoTableConfig;
class ULxItemEntryDefineTableConfig;
class ULxBuffDefineTableConfig;

// class ULxImageManage;


UCLASS(Blueprintable, DisplayName="数据表格管理对象")
class LXARPG_API ULxGameDataTablesManager : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Legacy table-config slots that are consumed directly by modules.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="输入信息表配置对象")
	TObjectPtr<ULxInputActionInfoTableConfig> m_pInputActionInfoTableConfig = nullptr;

	// 文本行样式配置数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig", DisplayName="文本行样式配置表")
	TObjectPtr<ULxTextLineStyleDataConfig> m_pTextLineStyleDataConfig = nullptr;

	// 角色属性规则定义表，Row Struct 使用 FLxAttributeData。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Attribute", DisplayName="角色属性信息表")
	TObjectPtr<UDataTable> m_pCharacterAttributeDataTable = nullptr;

	// 各种族的基础属性数值表，Key 为角色种族，Value 的 Row Struct 使用 FLxAttributeValueConfig。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Attribute", DisplayName="种族基础属性值表")
	TMap<ELxCharacterRaceType, TObjectPtr<UDataTable>> m_mapRaceAttributeValueConfigTables;

	// 属性增益词条表，Row Struct 使用 FLxEntryAttributeGain。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Entry", DisplayName="属性增益词条表")
	TObjectPtr<UDataTable> m_pAttributeGainEntryTable = nullptr;

	// 属性回复词条表，Row Struct 使用 FLxEntryAttributeRecovery。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Entry", DisplayName="属性回复词条表")
	TObjectPtr<UDataTable> m_pAttributeRecoveryEntryTable = nullptr;

	// 状态改变词条表，Row Struct 使用 FLxEntryChangeState。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Entry", DisplayName="状态改变词条表")
	TObjectPtr<UDataTable> m_pChangeStateEntryTable = nullptr;

	// 创建 Buff 词条表，Row Struct 使用 FLxEntryCreateBuff。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Entry", DisplayName="创建Buff词条表")
	TObjectPtr<UDataTable> m_pCreateBuffEntryTable = nullptr;

	// 多目标词条表，Row Struct 使用 FLxEntryMultiTarget。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Entry", DisplayName="多目标词条表")
	TObjectPtr<UDataTable> m_pMultiTargetEntryTable = nullptr;

	// 显示文本词条表，Row Struct 使用 FLxEntryDisplayText。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Entry", DisplayName="显示文本词条表")
	TObjectPtr<UDataTable> m_pDisplayTextEntryTable = nullptr;

	// 装备物品表，Row Struct 使用 FLxEquipmentInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Item", DisplayName="装备物品表")
	TObjectPtr<UDataTable> m_pEquipmentItemTable = nullptr;

	// 消耗品物品表，Row Struct 使用 FLxConsumableInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Item", DisplayName="消耗品物品表")
	TObjectPtr<UDataTable> m_pConsumableItemTable = nullptr;

	// 材料物品表，Row Struct 使用 FLxMaterialInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Item", DisplayName="材料物品表")
	TObjectPtr<UDataTable> m_pMaterialItemTable = nullptr;

	// Buff物品表，Row Struct 使用 FLxBuffInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="DataTableConfig|Item", DisplayName="Buff物品表")
	TObjectPtr<UDataTable> m_pBuffItemTable = nullptr;
	
	virtual void LoadDataTables();
};
