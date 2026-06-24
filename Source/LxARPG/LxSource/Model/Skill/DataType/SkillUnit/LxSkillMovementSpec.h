#pragma once

#include "CoreMinimal.h"
#include "LxSkillMovementSpec.generated.h"

/** 技能运动参数，配置侧统一使用 m、m/s、m/s²，运行时会转换为 UE 世界单位。 */
USTRUCT(BlueprintType, DisplayName="技能运动参数（m、m/s、m/s²）")
struct FLxSkillMovementSpec
{
	GENERATED_BODY()

	/** 移动速度，单位 m/s。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="移动速度（m/s）")
	float Speed = 0.0f;

	/** 移动加速度，单位 m/s²。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="移动加速度（m/s²）")
	float Acceleration = 0.0f;

	/** 最大移动距离，单位 m，常用于投射物、射线或移动区域。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="最大距离（m）")
	float MaxDistance = 0.0f;

	/** 重力倍率，主要用于抛射运动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="重力倍率")
	float GravityScale = 1.0f;

	/** 每次追踪修正时允许旋转的最大角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动|追踪", DisplayName="追踪最大转向角")
	float HomingMaxTurnAngle = 0.0f;

	/** 将配置侧米制距离转换为 UE 世界单位厘米。 */
	static constexpr float MeterToUnrealUnit(float MeterValue) { return MeterValue * 100.0f; }

	/** 获取 UE 内部使用的速度，单位 cm/s。 */
	float GetSpeedInUnrealUnits() const { return MeterToUnrealUnit(Speed); }

	/** 获取 UE 内部使用的加速度，单位 cm/s²。 */
	float GetAccelerationInUnrealUnits() const { return MeterToUnrealUnit(Acceleration); }

	/** 获取 UE 内部使用的最大距离，单位 cm。 */
	float GetMaxDistanceInUnrealUnits() const { return MeterToUnrealUnit(MaxDistance); }
};
