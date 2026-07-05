#pragma once

#include "CoreMinimal.h"
#include "LxSkillLobProjectileSpec.generated.h"

/** 抛射投射物专用参数，控制初始向上速度和下坠重力。 */
USTRUCT(BlueprintType, DisplayName="抛射投射物参数")
struct FLxSkillLobProjectileSpec
{
	GENERATED_BODY()

	/** 投射物激活时获得的初始向上速度，单位为 m/s。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|抛射", DisplayName="上抛速度（m/s）", meta=(ClampMin="0.0"))
	float UpwardLaunchSpeed = 5.0f;

	/** 世界重力倍率；0 表示不受重力影响，1 表示使用完整世界重力。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|抛射", DisplayName="重力系数", meta=(ClampMin="0.0"))
	float GravityScale = 1.0f;
};
