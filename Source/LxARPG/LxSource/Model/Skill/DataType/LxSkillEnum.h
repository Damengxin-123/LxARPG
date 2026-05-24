#pragma once

#include "CoreMinimal.h"
#include "LxSkillEnum.generated.h"

/** 技能释放类型，用于判断技能是直接释放还是需要蓄力释放。 */
UENUM(BlueprintType, DisplayName="技能释放类型")
enum class ELxSkillReleaseType : uint8
{
	None			UMETA(DisplayName="无"),
	DirectRelease	UMETA(DisplayName="直接释放"),
	ChargeRelease	UMETA(DisplayName="蓄力释放")
};
