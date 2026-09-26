#pragma once

#include "CoreMinimal.h"
#include "LxAITypes.generated.h"

/** AI根据稳定阵营规则和明确受击事件得到的目标关系。 */
UENUM(BlueprintType, DisplayName="AI目标关系")
enum class ELxAITargetRelation : uint8
{
	Ignore UMETA(DisplayName="无关"),
	Assist UMETA(DisplayName="友方"),
	Hostile UMETA(DisplayName="敌方")
};

/** 当前AI私有目标缓存记录感知信息时使用的来源类型。 */
UENUM(BlueprintType, DisplayName="AI感知来源")
enum class ELxAIPerceptionSource : uint8
{
	Unknown UMETA(DisplayName="未知"),
	Sight UMETA(DisplayName="视觉感知"),
	Range UMETA(DisplayName="范围感知"),
	Damage UMETA(DisplayName="受击感知"),
	Interaction UMETA(DisplayName="交互感知"),
	Effect UMETA(DisplayName="效果感知"),
	Hearing UMETA(DisplayName="听觉感知")
};
