// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxMaterialEnum.generated.h"

/**
 * @brief Enumerates the types of materials available.
 *
 * This enumeration is used to specify different material types. It is marked as Blueprintable,
 * allowing it to be used and referenced within Unreal Engine Blueprints.
 *
 * @note The enum values are intended to be expanded with more material types in future updates.
 */
UENUM(Blueprintable)
enum class ELxMaterialType : uint8
{
	None = 0 UMETA(DisplayName = "无"),
};
