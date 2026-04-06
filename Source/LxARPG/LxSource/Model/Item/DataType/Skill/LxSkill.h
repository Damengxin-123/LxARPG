// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxSkillCoreType.h"
#include "LxSkillEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "UObject/Object.h"
#include "LxSkill.generated.h"


/**
 * @brief 技能定义类型
 *
 * 该结构体继承自`FLxItemDefineBase`，用于在游戏中定义技能的相关信息。它包含了一个`FLxSkillCoreInfo`成员变量，用于存储技能的基础属性。
 */
USTRUCT(BlueprintType, DisplayName="技能定义类型")
struct FLxSkillDefine : public FLxItemDefineBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="技能基础属性")
	FLxSkillCoreInfo SkillCoreInfo;
};


/**
 * @brief 技能缓存类型
 *
 * 该结构体继承自`FLxItemDateBase`，用于在游戏中缓存技能的相关信息。它包含了一个`FLxSkillCoreInfo`成员变量，用于存储技能的基础属性。
 * 可以在编辑器中设置，并且可以通过蓝图读写。
 */
USTRUCT(BlueprintType, DisplayName="技能缓存类型")
struct FLxSkilllData : public FLxItemDateBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="技能基础属性")
	FLxSkillCoreInfo SkillCoreInfo;
};
