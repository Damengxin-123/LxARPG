#pragma once

#include "CoreMinimal.h"
#include "LxSkillAreaSpec.generated.h"

/** 范围效果子单元共有参数，只限制范围效果的最大存在时间。 */
USTRUCT(BlueprintType, DisplayName="范围效果参数")
struct FLxSkillAreaEffectSpec
{
	GENERATED_BODY()

	/** 范围效果最多持续多久，单位为秒；小于等于零表示不限制持续时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围效果", DisplayName="持续时间", meta=(ClampMin="0.0"))
	float Duration = 0.0f;
};

/** 持续型范围效果参数。 */
USTRUCT(BlueprintType, DisplayName="持续型范围效果参数")
struct FLxSkillDurationAreaEffectSpec
{
	GENERATED_BODY()

	/** 两次范围命中触发之间的间隔，单位为秒，最小为 0.1 秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围效果|持续型", DisplayName="触发间隔（秒）",
		meta=(ClampMin="0.1", UIMin="0.1"))
	float DetectionPeriod = 1.0f;
};

/** 缩放型范围效果参数。 */
USTRUCT(BlueprintType, DisplayName="缩放型范围效果参数")
struct FLxSkillScalingAreaEffectSpec
{
	GENERATED_BODY()

	/** 判定组件相对初始尺寸的最终缩放比例，持续时间内匀速到达该比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围效果|缩放型", DisplayName="缩放比例", meta=(ClampMin="0.0"))
	float ScaleRatio = 1.0f;
};
