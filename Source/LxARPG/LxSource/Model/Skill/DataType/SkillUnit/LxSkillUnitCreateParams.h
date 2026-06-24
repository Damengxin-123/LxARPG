#pragma once

#include "CoreMinimal.h"
#include "LxSkillAreaSpec.h"
#include "LxSkillAttachSpec.h"
#include "LxSkillAuraSpec.h"
#include "LxSkillHitLimitSpec.h"
#include "LxSkillLifeSpec.h"
#include "LxSkillMeleeSpec.h"
#include "LxSkillMovementSpec.h"
#include "LxSkillProjectileSpec.h"
#include "LxSkillPropagationSpec.h"
#include "LxSkillRaySpec.h"
#include "LxSkillSpawnEntitySpec.h"
#include "LxSkillSpaceSpec.h"
#include "LxSkillTargetFilterSpec.h"
#include "LxSkillTriggerSpec.h"
#include "LxSkillTriggerUnitSpec.h"
#include "LxSkillUnitCreateParams.generated.h"

class AActor;

/** 投射物技能单元创建参数，只暴露投射物自身需要的配置。 */
USTRUCT(BlueprintType, DisplayName="投射物技能单元创建参数")
struct FLxProjectileSkillUnitCreateParams
{
	GENERATED_BODY()

	/** 投射物运行参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="投射物参数")
	FLxSkillProjectileSpec ProjectileSpec;
};

/** 范围技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="范围技能单元创建参数")
struct FLxAreaSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="范围参数")
	FLxSkillAreaSpec AreaSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};

/** 移动范围技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="移动范围技能单元创建参数")
struct FLxMovingAreaSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="范围参数")
	FLxSkillAreaSpec AreaSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="运动参数")
	FLxSkillMovementSpec MovementSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};

/** 近战技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="近战技能单元创建参数")
struct FLxMeleeSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="近战参数")
	FLxSkillMeleeSpec MeleeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};

/** 射线技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="射线技能单元创建参数")
struct FLxRaySkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="射线参数")
	FLxSkillRaySpec RaySpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="运动参数")
	FLxSkillMovementSpec MovementSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="传播参数")
	FLxSkillPropagationSpec PropagationSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};

/** 生成实体类技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="生成实体技能单元创建参数")
struct FLxSpawnEntitySkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生成实体参数")
	FLxSkillSpawnEntitySpec SpawnEntitySpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;
};

/** 触发器技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="触发器技能单元创建参数")
struct FLxTriggerSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发器参数")
	FLxSkillTriggerUnitSpec TriggerUnitSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};

/** 附着技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="附着技能单元创建参数")
struct FLxAttachSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="附着参数")
	FLxSkillAttachSpec AttachSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="附着目标")
	TObjectPtr<AActor> AttachTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;
};

/** 光环技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="光环技能单元创建参数")
struct FLxAuraSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="光环参数")
	FLxSkillAuraSpec AuraSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};
