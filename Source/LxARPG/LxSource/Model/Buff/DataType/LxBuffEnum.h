// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxBuffEnum.generated.h"

/**
 * @brief 枚举定义了游戏中不同类型的buff效果。
 *
 * 该枚举用于分类和标识游戏中的各种buff效果。每个枚举值代表一种特定的buff类型，并带有用户界面用途的显示名称，以便在Unreal Engine编辑器中更易于阅读和使用。
 *
 * @ingroup GameMechanics
 */
UENUM(BlueprintType)
enum class ELxBuffType : uint8
{
	None				UMETA(DisplayName="无"),
	AttributeModify		UMETA(DisplayName="属性修改型"),
	AttributeRecovery	UMETA(DisplayName="属性回复型"),
	StateModify			UMETA(DisplayName="状态修改型"),
};

/**
 * @brief 枚举游戏中不同类型的buff的ID。
 *
 * 该枚举用于唯一标识可以应用于游戏中的角色或对象的各种buff效果。每个枚举值对应一种特定类型的buff，并带有用户界面用途的显示名称。
 *
 * @note 这些值设计用于蓝图中，并具有自定义显示名称，以便在Unreal Engine编辑器中更易于阅读。
 *
 * @ingroup GameMechanics
 */
UENUM(BlueprintType, DisplayName="buff的ID")
enum class ELxBuffID : uint8
{
	None						UMETA(DisplayName="无"),
	HPRestore					UMETA(DisplayName="生命回复"),
	MPRestore					UMETA(DisplayName="魔力回复"),
	AttackImprove 				UMETA(DisplayName="攻击力提升"),
};
