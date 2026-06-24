#pragma once

#include "CoreMinimal.h"
#include "LxSkillSpaceSpec.generated.h"

/** 技能空间参数，配置侧统一使用 m，运行时会转换为 UE 世界单位。 */
USTRUCT(BlueprintType, DisplayName="技能空间参数（m）")
struct FLxSkillSpaceSpec
{
	GENERATED_BODY()

	/** 外半径，单位 m，用于圆形、球形、扇形、环形、胶囊体等形状。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="外半径（m）")
	float Radius = 0.0f;

	/** 内半径，单位 m，主要用于环形范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="内半径（m）")
	float InnerRadius = 0.0f;

	/** 长度，单位 m，用于矩形、线形、胶囊体或锥形范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="长度（m）")
	float Length = 0.0f;

	/** 宽度，单位 m，用于矩形、线形宽度或其他扁平范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="宽度（m）")
	float Width = 0.0f;

	/** 高度，单位 m，用于限制垂直方向上的有效判定范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="高度（m）")
	float Height = 0.0f;

	/** 角度，单位度，用于扇形或锥形范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="角度（度）")
	float Angle = 0.0f;

	/** 将配置侧米制距离转换为 UE 世界单位厘米。 */
	static constexpr float MeterToUnrealUnit(float MeterValue) { return MeterValue * 100.0f; }

	/** 获取 UE 内部使用的外半径，单位 cm。 */
	float GetRadiusInUnrealUnits() const { return MeterToUnrealUnit(Radius); }

	/** 获取 UE 内部使用的内半径，单位 cm。 */
	float GetInnerRadiusInUnrealUnits() const { return MeterToUnrealUnit(InnerRadius); }

	/** 获取 UE 内部使用的长度，单位 cm。 */
	float GetLengthInUnrealUnits() const { return MeterToUnrealUnit(Length); }

	/** 获取 UE 内部使用的宽度，单位 cm。 */
	float GetWidthInUnrealUnits() const { return MeterToUnrealUnit(Width); }

	/** 获取 UE 内部使用的高度，单位 cm。 */
	float GetHeightInUnrealUnits() const { return MeterToUnrealUnit(Height); }
};
