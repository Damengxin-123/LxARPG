// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableBase.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTypes.h"
#include "LxCharacterRaceConfig.h"
#include "LxGameDataTablesManager.generated.h"

class UDataTable;


UCLASS(Blueprintable, DisplayName="数据表格管理对象")
class LXARPG_API ULxGameDataTablesManager : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 可玩种族与玩家角色子类的唯一映射，统一用于创建、预览和加载存档。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|角色", DisplayName="角色种族表",
		meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxCharacterRaceConfig"))
	TObjectPtr<UDataTable> CharacterRaceTable = nullptr;

	/** 验证并读取全部可玩种族，拒绝重复种族、无效名称及不可生成的角色类。 */
	UFUNCTION(BlueprintCallable, Category="数据表配置|角色", DisplayName="获取可玩种族配置")
	bool GetCharacterRaceConfigs(TArray<FLxCharacterRaceConfig>& OutConfigs, FString& OutError) const;

	/** 按存档种族查询唯一的角色配置，不使用数据表行名称。 */
	UFUNCTION(BlueprintCallable, Category="数据表配置|角色", DisplayName="查询角色种族配置")
	bool GetCharacterRaceConfig(ELxCharacterRaceType Race, FLxCharacterRaceConfig& OutConfig, FString& OutError) const;

	// 输入行为信息表，Row Struct 使用 FLxInputActionInfo。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|输入", DisplayName="输入行为信息表")
	TObjectPtr<UDataTable> m_pInputActionInfoTableConfig = nullptr;

	// 属性增益词条表，Row Struct 使用 FLxEntryAttributeGain。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="属性增益词条表")
	TObjectPtr<UDataTable> m_pAttributeGainEntryTable = nullptr;

	/** 属性影响词条表，Row Struct（行结构）使用 FLxEntryAttributeInfluence。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="属性影响词条表",
		meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxEntryAttributeInfluence"))
	TObjectPtr<UDataTable> m_pAttributeInfluenceEntryTable = nullptr;

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

	/** 赋予职业词条表，Row Struct（行结构）使用 FLxEntryGrantProfession。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|词条", DisplayName="赋予职业词条表")
	TObjectPtr<UDataTable> m_pGrantProfessionEntryTable = nullptr;

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

	/** 任务系列静态资产索引表，首版由任务静态数据模块读取并建立软引用映射。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|任务", DisplayName="任务系列索引表",
		meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxQuestSeriesRegistryRow"))
	TObjectPtr<UDataTable> m_pQuestSeriesIndexTable = nullptr;

	// 富文本样式映射表，Row Struct 使用 FLxRichTextStyleRow，内部行引用指向 FRichTextStyleRow 样式表。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="数据表配置|富文本样式", DisplayName="富文本样式映射表", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxRichTextStyleRow"))
	TObjectPtr<UDataTable> m_pRichTextStyleTable = nullptr;
	
	virtual void LoadDataTables();
};
