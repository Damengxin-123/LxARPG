#pragma once

#include "CoreMinimal.h"
#include "LxSkillSpaceSpec.generated.h"

/** 技能空间参数，描述本次释放需要覆盖的判定尺寸。具体形状通常由技能单元Actor类型自身限定。 */
USTRUCT(BlueprintType, DisplayName="技能空间参数")
struct FLxSkillSpaceSpec
{
	GENERATED_BODY()

	/** 外半径，用于圆形、球形、扇形、环形、胶囊体等形状。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="外半径")
	float Radius = 0.0f;

	/** 内半径，主要用于环形范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="内半径")
	float InnerRadius = 0.0f;

	/** 长度，用于矩形、线形、胶囊体或锥形范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="长度")
	float Length = 0.0f;

	/** 宽度，用于矩形、线形宽度或其他扁平范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="宽度")
	float Width = 0.0f;

	/** 高度，用于限制垂直方向上的有效判定范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="高度")
	float Height = 0.0f;

	/** 角度，用于扇形或锥形范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|空间", DisplayName="角度")
	float Angle = 0.0f;
};
