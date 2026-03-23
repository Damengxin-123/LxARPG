// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxSkillEnum.h"
#include "LxARPG/LxSource/Model/Item/ItemBase/LxItemDataType.h"
#include "UObject/Object.h"
#include "LxSkillData.generated.h"

/**
 * @brief FLxSkillData 结构体定义了一个技能数据结构，继承自FLxItemBase。
 * 该结构体用于表示游戏中的技能，并包含技能类型等信息。
 */
USTRUCT(BlueprintType)
struct FLxSkillData : public FLxItemBase
{
	GENERATED_BODY()

	ELxSkillType SkillType = ELxSkillType::None;

	FLxSkillData()
	{
		ItemType = ELxItemType::Skill;
	}

	virtual void InitData() override{};
};
