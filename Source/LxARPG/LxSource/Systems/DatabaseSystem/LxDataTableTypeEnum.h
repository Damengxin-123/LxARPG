// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxDataTableTypeEnum.generated.h"

UENUM()
enum class ELxDataTableTypeEnum : uint8
{
	InputActionInfo			UMETA(DisplayName="输入行为信息表"),
	CharacterAttribute		UMETA(DisplayName="角色属性设置表"),
	EquipmentData			UMETA(DisplayName="装备属性表"),
	ConsumableData			UMETA(DisplayName="消耗品属性表"),
	MaterialData			UMETA(DisplayName="材料属性表"),
};

