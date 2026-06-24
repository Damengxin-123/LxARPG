#pragma once

#include "CoreMinimal.h"
#include "LxSkillProjectileSpec.generated.h"

class AActor;

/** 投射物触发行为上下文，用于把有效命中目标和后续子单元创建位置回传给技能。 */
USTRUCT(BlueprintType, DisplayName="投射物触发行为上下文")
struct FLxProjectileTriggerContext
{
	GENERATED_BODY()

	/** 本次触发命中的有效目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|事件", DisplayName="命中目标")
	TObjectPtr<AActor> HitTarget = nullptr;

	/** 后续技能子单元可使用的创建坐标和朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|事件", DisplayName="创建变换")
	FTransform SpawnTransform = FTransform::Identity;
};

/** 投射物失效上下文，用于把失效位置回传给技能。 */
USTRUCT(BlueprintType, DisplayName="投射物失效上下文")
struct FLxProjectileInvalidationContext
{
	GENERATED_BODY()

	/** 投射物失效时的坐标和朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|事件", DisplayName="失效变换")
	FTransform SpawnTransform = FTransform::Identity;
};

/** 投射物技能单元运行参数，只保留投射物创建时真正需要覆盖的基础控制参数。 */
USTRUCT(BlueprintType, DisplayName="投射物技能参数（m、m/s、m/s²）")
struct FLxSkillProjectileSpec
{
	GENERATED_BODY()

	/** 投射物初始飞行速度，单位 m/s。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|飞行", DisplayName="飞行速度（m/s）")
	float FlightSpeed = 0.0f;

	/** 投射物飞行加速度，单位 m/s²。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|飞行", DisplayName="飞行加速度（m/s²）")
	float FlightAcceleration = 0.0f;

	/** 投射物最大飞行距离，单位 m，达到后会进入失效流程。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|飞行", DisplayName="最大飞行距离（m）")
	float MaxFlightDistance = 0.0f;

	/** 可穿透的角色目标数量，碰到非角色实体会直接视为障碍物。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|命中", DisplayName="穿透数量")
	int32 MaxPierceCount = 0;

	/** 一次创建时发射的投射物数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|创建", DisplayName="发射数量")
	int32 LaunchCount = 1;

	/** 投射物失效后是否自动销毁。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|生命周期", DisplayName="失效后销毁")
	bool bDestroyAfterInvalidated = true;
};
