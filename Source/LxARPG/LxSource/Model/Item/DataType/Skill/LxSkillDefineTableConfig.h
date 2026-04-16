// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxDataTableConfigBase.h"
#include "LxSkill.h"
#include "LxSkillDefineTableConfig.generated.h"

class UDataTable;

UCLASS(Blueprintable, DisplayName="技能定义表格配置")
class LXARPG_API ULxSkillDefineTableConfig : public ULxDataTableConfigBase
{
	GENERATED_BODY()

public:
	virtual void InitDataTableLoading() override;

	const FLxSkillDefine* GetSkillDefine(const FName& InItemID) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Skill Define Table List")
	TArray<UDataTable*> m_vSkillDefineTableList;

private:
	TMap<FName, FLxSkillDefine> m_tSkillDefineMap;
};
