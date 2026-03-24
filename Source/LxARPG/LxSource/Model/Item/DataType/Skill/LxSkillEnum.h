// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxSkillEnum.generated.h"


/**
 * @enum ELxSkillType
 * @brief 技能类型。
 *
 * 该枚举用于区分技能类物品的子类型。
 * 当前仅保留默认占位值，便于后续扩展。
 *
 * @note 可在蓝图中使用。
 */
UENUM(BlueprintType)
enum class ELxSkillType : uint8
{
	/** 无 */
	None,
};
