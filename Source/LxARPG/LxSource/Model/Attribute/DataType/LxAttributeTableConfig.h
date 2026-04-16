// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxAttributeTableConfig.generated.h"

/**
 * @class ULxAttributeTableConfig
 * @brief A class for configuring and managing attribute data tables, extending the functionality of ULxDataTableConfigBase.
 *
 * This class is designed to provide a framework for loading and handling specific attribute data within a game or application. It inherits from ULxDataTableConfigBase, allowing it to leverage base functionalities while potentially adding or customizing features related to attribute management.
 */
UCLASS(Blueprintable, DisplayName="属性表格配置")
class LXARPG_API ULxAttributeTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()
public:
	virtual void InitDataTableLoading() override;

	const TArray<FLxAttributeData>* GetAttributeDataList(ELxCharacterRaceType InRaceType) const;

protected:
	// 各种族角色属性配置表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="各种族角色默认属性配置表")
	TMap<ELxCharacterRaceType, UDataTable*> m_mapAttributeTableList;
private:

	TMap<ELxCharacterRaceType, TArray<FLxAttributeData>> m_tAttributeDataMap;
};
