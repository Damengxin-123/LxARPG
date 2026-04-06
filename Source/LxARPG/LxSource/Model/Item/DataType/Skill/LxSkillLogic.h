// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxSkill.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxSkillLogic.generated.h"

UCLASS()
class LXARPG_API ULxSkillLogic : public ULxItemLogicBase
{
	GENERATED_BODY()

public:
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;
	virtual const FLxItemDateBase* GetItemDataBase() const override;
	virtual bool UseItem() override;
	virtual bool ItemIsStack() override;
	virtual bool StackItem(ULxItemLogicBase* SourceItemLogic) override;
	virtual bool ItemIsValid() override;
	virtual bool operator<(const ULxItemLogicBase* Other) const override;
	virtual bool operator>(const ULxItemLogicBase* Other) const override;

private:
	FLxSkilllData m_SkillData;
};
