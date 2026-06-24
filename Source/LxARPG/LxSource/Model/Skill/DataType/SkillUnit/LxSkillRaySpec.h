#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillRaySpec.generated.h"

/** 射线与持续射线独有运行时参数，最大长度、持续时间和周期判定由通用结构体提供。 */
USTRUCT(BlueprintType, DisplayName="射线技能形态参数（宽度m）")
struct FLxSkillRaySpec
{
	GENERATED_BODY()

	/** 射线宽度，单位 m，等于 0 时可按普通线检测处理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|射线", DisplayName="射线宽度（m）")
	float RayWidth = 0.0f;

	/** 射线距离衰减方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|射线", DisplayName="衰减方式")
	ELxSkillRayFalloffType FalloffType = ELxSkillRayFalloffType::None;

	/** 到达最大距离时保留的强度倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|射线", DisplayName="最大距离强度倍率")
	float MaxRangeFalloffMultiplier = 1.0f;

	/** 将配置侧米制距离转换为 UE 世界单位厘米。 */
	static constexpr float MeterToUnrealUnit(float MeterValue) { return MeterValue * 100.0f; }

	/** 获取 UE 内部使用的射线宽度，单位 cm。 */
	float GetRayWidthInUnrealUnits() const { return MeterToUnrealUnit(RayWidth); }
};
