// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxMaterial.h"
#include "LxMaterialDefineTableConfig.generated.h"

class UDataTable;

UCLASS(Blueprintable, DisplayName="材料定义表格配置")
class LXARPG_API ULxMaterialDefineTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxMaterialDefine* GetMaterialDefine(const FName& InItemID) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Material Define Table List")
	TArray<UDataTable*> m_vMaterialDefineTableList;

private:
	TMap<FName, FLxMaterialDefine> m_tMaterialDefineMap;
};
