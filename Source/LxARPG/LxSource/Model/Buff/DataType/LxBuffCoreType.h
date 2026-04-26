// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuffEnum.h"
#include "UObject/Object.h"
#include "LxBuffCoreType.generated.h"

/**
 * @struct FLxBuffCoreInfo
 * @brief 包含了Buff的基础信息，用于在游戏中定义和配置不同的Buff效果。
 *
 * 该结构体包含了Buff的类型、ID、是否直接显示以及是否可以重复等属性。这些属性可以通过编辑器或蓝图脚本进行访问和修改，
 * 从而实现对不同Buff效果的灵活配置。
 *
 * @note 此结构体标记为BlueprintType，允许在蓝图中使用。
 *
 * @see USTRUCT, GENERATED_BODY, ELxBuffType, ELxBuffID
 */
USTRUCT(BlueprintType, DisplayName="Buff基础信息")
struct FLxBuffCoreInfo
{
	GENERATED_BODY()

	/**
	 * @var ELxBuffType BuffType
	 * @brief 定义了Buff的类型，用于区分不同的Buff效果类别。
	 *
	 * 该枚举变量指定了Buff可以是属性修改型、属性回复型或状态修改型。通过编辑器或蓝图脚本可访问和修改此属性，
	 * 从而配置不同类型的Buff效果。
	 *
	 * @note 此变量标记为BlueprintReadWrite，允许在蓝图中进行读写操作。
	 *
	 * @see UPROPERTY, GENERATED_BODY, ELxBuffType
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff基础信息", DisplayName="Buff类型")
	ELxBuffType BuffType = ELxBuffType::None;

	/**
	 * @var ELxBuffID BuffID
	 * @brief 用于标识具体的Buff实例，通过唯一ID来区分不同的Buff效果。
	 *
	 * 该枚举变量定义了游戏内可用的各种Buff的ID，如生命回复、魔力回复和攻击力提升等。通过编辑器或蓝图脚本可以访问和修改此属性，
	 * 以便为游戏角色或其他对象配置特定的Buff效果。
	 *
	 * @note 此变量标记为BlueprintReadWrite，允许在蓝图中进行读写操作。
	 *
	 * @see UPROPERTY, GENERATED_BODY, ELxBuffID
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff基础信息", DisplayName="Buff类型")
	ELxBuffID  BuffID = ELxBuffID::None;

	/**
	 * @var bool IsShow
	 * @brief 控制Buff是否在游戏中直接显示给玩家。
	 *
	 * 该布尔变量决定了特定的Buff效果是否在游戏界面中直接对玩家可见。通过编辑器或蓝图脚本可以访问和修改此属性，
	 * 从而根据需要调整Buff的显示行为，例如某些隐藏的增益效果可能不需要直接展示给玩家。
	 *
	 * @note 此变量标记为BlueprintReadWrite，允许在蓝图中进行读写操作。
	 *
	 * @see UPROPERTY, GENERATED_BODY
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff基础信息", DisplayName="Buff是否直接显示")
	bool IsShow = false;

	/**
	 * @var bool IsRepeat
	 * @brief 控制Buff是否可以在同一目标上重复叠加。
	 *
	 * 该布尔变量决定了特定的Buff效果是否可以多次施加于同一个目标。如果设置为`true`，则允许Buff在满足条件时重复叠加；如果设置为`false`，则Buff在同一目标上只能存在一次。
	 * 通过编辑器或蓝图脚本可以访问和修改此属性，从而根据游戏设计需求灵活配置Buff的行为。
	 *
	 * @note 此变量标记为BlueprintReadWrite，允许在蓝图中进行读写操作。
	 *
	 * @see UPROPERTY, GENERATED_BODY
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff基础信息", DisplayName="Buff是否可以重复")
	bool IsRepeat = false;
	
};

/**
 * @struct FLxBuffEffectInfo
 * @brief 表示Buff效果信息的结构体，用于存储与Buff相关的属性。
 *
 * 该结构体包含两个主要属性：Duration和Proportion，分别表示Buff的持续时间和效果比例。
 * 可以通过编辑器或蓝图脚本访问和修改这些属性。
 *
 * @note 此结构体标记为BlueprintType，允许在蓝图中使用。
 *
 * @see UPROPERTY, GENERATED_BODY
 */
USTRUCT(BlueprintType, DisplayName="Buff效果信息")
struct FLxBuffEffectInfo
{
	GENERATED_BODY()


	/**
	 * @var float Duration
	 * @brief 表示Buff效果的持续时间，单位为秒。
	 *
	 * 当时间为负数时，表示一直持续，直到被动失效
	 * 该变量用于定义特定Buff效果能够作用的时间长度。通过编辑器或蓝图脚本可以直接访问和修改此属性，以调整游戏中的Buff效果持续时间。
	 *
	 * @note 变量标记为EditAnywhere和BlueprintReadWrite，意味着它既可以在编辑器中被编辑，也可以在蓝图中被读取和写入。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff效果信息", DisplayName="buff持续时间-秒")
	float Duration = 0;


	/**
	 * @var float EffectivePeriod
	 * @brief 表示Buff效果的生效周期，单位为秒。
	 *
	 * 该变量定义了特定Buff效果每隔多久生效一次。通过编辑器或蓝图脚本可以直接访问和修改此属性，以调整游戏中的Buff效果生效频率。
	 *
	 * @note 变量标记为EditAnywhere和BlueprintReadWrite，意味着它既可以在编辑器中被编辑，也可以在蓝图中被读取和写入。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff效果信息", DisplayName="buff生效周期-秒")
	float EffectivePeriod = 0;

	/**
	 * @var float Proportion
	 * @brief 表示Buff效果的比例，用于调整Buff作用的强度。
	 *
	 * 该变量定义了Buff效果的具体数值比例，影响着Buff对角色或其他游戏元素的实际影响程度。通过编辑器或蓝图脚本可以直接访问和修改此属性，以调整游戏中的Buff效果强度。
	 *
	 * @note 变量标记为EditAnywhere和BlueprintReadWrite，意味着它既可以在编辑器中被编辑，也可以在蓝图中被读取和写入。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff效果信息", DisplayName="buff效果比例")
	float Proportion = 0;

};
