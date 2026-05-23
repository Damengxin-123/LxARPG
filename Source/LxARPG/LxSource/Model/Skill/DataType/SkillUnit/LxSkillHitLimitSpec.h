#pragma once

#include "CoreMinimal.h"
#include "LxSkillHitLimitSpec.generated.h"

/** 技能命中限制参数，描述目标成为合法目标后能被命中多少次以及命中间隔。 */
USTRUCT(BlueprintType, DisplayName="技能命中限制参数")
struct FLxSkillHitLimitSpec
{
	GENERATED_BODY()

	/** 总命中次数限制，低于等于 0 表示不限制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|命中限制", DisplayName="总命中次数")
	int32 MaxTotalHitCount = 0;

	/** 单个目标最大命中次数，低于等于 0 表示不限制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|命中限制", DisplayName="单目标命中次数")
	int32 MaxHitCountPerTarget = 1;

	/** 同一个目标两次命中之间的间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|命中限制", DisplayName="单目标命中间隔")
	float HitIntervalPerTarget = 0.0f;

	/** 是否允许同一个目标被重复命中。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|命中限制", DisplayName="允许重复命中同目标")
	bool bCanHitSameTargetAgain = false;

	/** 是否忽略已经命中过的目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|命中限制", DisplayName="忽略已命中目标")
	bool bIgnoreAlreadyHitTargets = true;
};
