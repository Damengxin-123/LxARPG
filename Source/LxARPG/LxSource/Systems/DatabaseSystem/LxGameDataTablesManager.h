// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxGameDataTablesManager.generated.h"

class UDataTable;
class ULxDataTableConfigBase;
class ULxTextLineStyleDataConfig;
class ULxItemEntryDefineTableConfig;
class ULxBuffDefineTableConfig;

// class ULxImageManage;


UCLASS(Blueprintable, DisplayName="数据表格管理对象")
class LXARPG_API ULxGameDataTablesManager : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 输入行为信息表，Row Struct 使用 FLxInputActionInfo。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|输入", DisplayName="输入行为信息表")
	TObjectPtr<UDataTable> m_pInputActionInfoTableConfig = nullptr;

	// 角色属性规则定义表，Row Struct 使用 FLxAttributeData。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|角色属性", DisplayName="角色属性信息表")
	TObjectPtr<UDataTable> m_pCharacterAttributeDataTable = nullptr;

	// 各种族的基础属性数值表，Key 为角色种族，Value 的 Row Struct 使用 FLxAttributeValueConfig。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|角色属性", DisplayName="种族基础属性值表")
	TMap<ELxCharacterRaceType, TObjectPtr<UDataTable>> m_mapRaceAttributeValueConfigTables;

	// 属性增益词条表，Row Struct 使用 FLxEntryAttributeGain。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="属性增益词条表")
	TObjectPtr<UDataTable> m_pAttributeGainEntryTable = nullptr;

	// 属性回复词条表，Row Struct 使用 FLxEntryAttributeRecovery。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="属性回复词条表")
	TObjectPtr<UDataTable> m_pAttributeRecoveryEntryTable = nullptr;

	// 状态改变词条表，Row Struct 使用 FLxEntryChangeState。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="状态改变词条表")
	TObjectPtr<UDataTable> m_pChangeStateEntryTable = nullptr;

	// 创建 Buff 词条表，Row Struct 使用 FLxEntryCreateBuff。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="创建Buff词条表")
	TObjectPtr<UDataTable> m_pCreateBuffEntryTable = nullptr;

	// 多目标词条表，Row Struct 使用 FLxEntryMultiTarget。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="多目标词条表")
	TObjectPtr<UDataTable> m_pMultiTargetEntryTable = nullptr;

	// 显示文本词条表，Row Struct 使用 FLxEntryDisplayText。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="显示文本词条表")
	TObjectPtr<UDataTable> m_pDisplayTextEntryTable = nullptr;

	// 授予技能词条表，Row Struct 使用 FLxEntryGrantSkill。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="授予技能词条表")
	TObjectPtr<UDataTable> m_pGrantSkillEntryTable = nullptr;

	// 造成伤害词条表，Row Struct 使用 FLxEntryDamage。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="造成伤害词条表")
	TObjectPtr<UDataTable> m_pDamageEntryTable = nullptr;

	// 装备物品表，Row Struct 使用 FLxEquipmentInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|物品", DisplayName="装备物品表")
	TObjectPtr<UDataTable> m_pEquipmentItemTable = nullptr;

	// 消耗品物品表，Row Struct 使用 FLxConsumableInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|物品", DisplayName="消耗品物品表")
	TObjectPtr<UDataTable> m_pConsumableItemTable = nullptr;

	// 材料物品表，Row Struct 使用 FLxMaterialInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|物品", DisplayName="材料物品表")
	TObjectPtr<UDataTable> m_pMaterialItemTable = nullptr;

	// Buff物品表，Row Struct 使用 FLxBuffInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|物品", DisplayName="Buff物品表")
	TObjectPtr<UDataTable> m_pBuffItemTable = nullptr;

	// 技能物品表，Row Struct 使用 FLxSkillItemInformation。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|物品", DisplayName="技能物品表")
	TObjectPtr<UDataTable> m_pSkillItemTable = nullptr;

	// 角色职业表，Row Struct 使用 FLxProfessionDefinitionTableRow。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|职业", DisplayName="角色职业表")
	TObjectPtr<UDataTable> m_pProfessionDefinitionTable = nullptr;

	// 富文本样式映射表，Row Struct 使用 FLxRichTextStyleRow，内部行引用指向 FRichTextStyleRow 样式表。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|富文本样式", DisplayName="富文本样式映射表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxRichTextStyleRow"))
	TObjectPtr<UDataTable> m_pRichTextStyleTable = nullptr;
	
	virtual void LoadDataTables();
};
