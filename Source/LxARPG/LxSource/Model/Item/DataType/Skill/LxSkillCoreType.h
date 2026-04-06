// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxSkillEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "UObject/Object.h"
#include "LxSkillCoreType.generated.h"

/**
 * @brief 定义了技能的基础属性信息。
 *
 * 该结构体用于存储和管理与技能相关的基本属性，如技能类型等。通过BlueprintType标记，可以在蓝图中直接使用和编辑此结构体。
 * 技能的类型由ELxSkillType枚举定义，允许用户指定或更改技能的具体类型。
 *
 * @note 此结构体主要应用于游戏开发中的技能系统，帮助开发者更方便地管理和操作技能数据。
 */
USTRUCT(BlueprintType, DisplayName="技能基础属性")
struct FLxSkillCoreInfo
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="技能类型")
	ELxSkillType SkillType;
};


