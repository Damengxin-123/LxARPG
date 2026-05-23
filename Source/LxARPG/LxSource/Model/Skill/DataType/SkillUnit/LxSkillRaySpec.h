#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillRaySpec.generated.h"

/** 射线与持续射线独有运行时参数，最大长度、持续时间和周期判定由通用结构体提供。 */
USTRUCT(BlueprintType, DisplayName="射线技能形态参数")
struct FLxSkillRaySpec
{
	GENERATED_BODY()

	/** 射线宽度，等于 0 时可按普通线检测处理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|射线", DisplayName="射线宽度")
	float RayWidth = 0.0f;

	/** 射线距离衰减方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|射线", DisplayName="衰减方式")
	ELxSkillRayFalloffType FalloffType = ELxSkillRayFalloffType::None;

	/** 到达最大距离时保留的强度倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|射线", DisplayName="最大距离强度倍率")
	float MaxRangeFalloffMultiplier = 1.0f;
};
