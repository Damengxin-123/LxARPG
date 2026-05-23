#pragma once

#include "CoreMinimal.h"
#include "LxSkillProjectileSpec.generated.h"

/** 投射物技能独有运行时参数。投射物Actor类型和视觉效果由技能单元类型自身决定。 */
USTRUCT(BlueprintType, DisplayName="投射物技能形态参数")
struct FLxSkillProjectileSpec
{
	GENERATED_BODY()

	/** 投射物碰撞半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|投射物", DisplayName="碰撞半径")
	float CollisionRadius = 0.0f;

	/** 一次释放时创建的投射物数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|投射物", DisplayName="发射数量")
	int32 LaunchCount = 1;

	/** 多个投射物发射时的散射总角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|投射物", DisplayName="散射角度")
	float LaunchSpreadAngle = 0.0f;

	/** 多个投射物之间的发射间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|投射物", DisplayName="发射间隔")
	float LaunchInterval = 0.0f;
};
