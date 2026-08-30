// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LxBlockChunkTypes.generated.h"

/** 方块区块中可以保存和显示的基础方块类型。 */
UENUM(BlueprintType, meta=(DisplayName="方块类型"))
enum class ELxBlockType : uint8
{
	/** 空气不生成任何可见表面或碰撞。 */
	Air UMETA(DisplayName="空气"),

	/** 草地方块用于地形每一列最上方的表层。 */
	Grass UMETA(DisplayName="草地方块"),

	/** 泥土方块用于草地表层下方的浅层土壤。 */
	Dirt UMETA(DisplayName="泥土方块"),

	/** 岩石方块用于地形较深位置的主体。 */
	Stone UMETA(DisplayName="岩石方块")
};
