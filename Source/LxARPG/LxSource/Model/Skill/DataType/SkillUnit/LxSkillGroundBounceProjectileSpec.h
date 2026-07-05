#pragma once

#include "CoreMinimal.h"
#include "LxSkillGroundBounceProjectileSpec.generated.h"

/** 地面弹跳投射物专用参数，只控制地面弹跳轨迹。 */
USTRUCT(BlueprintType, DisplayName="地面弹跳投射物参数")
struct FLxSkillGroundBounceProjectileSpec
{
	GENERATED_BODY()

	/** 投射物能够从地面弹起的总次数；次数耗尽后再次落地将进入失效流程。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|地面弹跳", DisplayName="地面弹跳次数", meta=(ClampMin="0"))
	int32 MaxGroundBounceCount = 0;

	/** 世界重力倍率；0 表示不受重力影响，1 表示使用完整世界重力。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|地面弹跳", DisplayName="重力系数", meta=(ClampMin="0.0"))
	float GravityScale = 1.0f;
};
