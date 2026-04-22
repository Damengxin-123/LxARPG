// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxItemEntryData.h"
#include "LxItemEntryDefineTableConfig.generated.h"

class UDataTable;

/**
 * @brief 词条定义数据表配置
 *
 * 支持配置多张 FLxItemEntryDefine 类型数据表，并在初始化时缓存到 EntryID 索引表中。
 */
UCLASS(Blueprintable, DisplayName="词条定义表格配置")
class LXARPG_API ULxItemEntryDefineTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxItemEntryDefine* GetItemEntryDefine(const FName& InEntryID) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条定义配置表列表")
	TArray<UDataTable*> m_vItemEntryDefineTableList;

private:
	TMap<FName, FLxItemEntryDefine> m_tItemEntryDefineMap;
};
