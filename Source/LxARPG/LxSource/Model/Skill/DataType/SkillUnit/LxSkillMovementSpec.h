#pragma once

#include "CoreMinimal.h"
#include "LxSkillMovementSpec.generated.h"

/** 技能运动参数，描述本次释放需要覆盖的移动数值。具体运动方式由技能单元Actor类型自身限定。 */
USTRUCT(BlueprintType, DisplayName="技能运动参数")
struct FLxSkillMovementSpec
{
	GENERATED_BODY()

	/** 移动速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="移动速度")
	float Speed = 0.0f;

	/** 移动加速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="移动加速度")
	float Acceleration = 0.0f;

	/** 最大移动距离，常用于投射物、射线或移动区域。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="最大距离")
	float MaxDistance = 0.0f;

	/** 重力倍率，主要用于抛射运动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="重力倍率")
	float GravityScale = 1.0f;

	/** 每次追踪修正时允许旋转的最大角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动|追踪", DisplayName="追踪最大转向角")
	float HomingMaxTurnAngle = 0.0f;
};
