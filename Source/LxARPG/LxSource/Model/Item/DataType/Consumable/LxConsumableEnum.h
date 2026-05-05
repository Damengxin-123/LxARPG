// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxConsumableEnum.generated.h"

UENUM(BlueprintType, DisplayName="消耗品类型")
enum class ELxConsumableType : uint8
{
	// 无类型
	None = 0						UMETA(DisplayName="无类型"),
	// 回复类
	Recovery	= 0b00000001		UMETA(DisplayName="回复类"),
	// 功能类
	Functional	= 0b00000010		UMETA(DisplayName="功能类"),

	// 可消耗 
	Consumable	= 0b00000100		UMETA(Hidden, DisplayName="可消耗"),
	// 不可消耗
	NotConsumable = 0b00001000		UMETA(Hidden, DisplayName="不可消耗"),

	// 可消耗恢复类
	ConsumableAndRecovery		= Recovery|Consumable		UMETA(Hidden, DisplayName="可消耗恢复类"),
	// 可消耗功能类
	ConsumableAndFunctional		= Functional|Consumable		UMETA(Hidden, DisplayName="可消耗功能类"),
	// 不可消耗恢复类
	NotConsumableAndRecovery	= NotConsumable|Recovery	UMETA(Hidden, DisplayName="不可消耗恢复类"),
	// 不可消耗功能类
	NotConsumableAndFunctional	= NotConsumable|Functional  UMETA(Hidden, DisplayName="不可消耗功能类"),
	
};
