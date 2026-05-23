#pragma once

#include "CoreMinimal.h"
#include "LxSkillHitLimitSpec.h"
#include "LxSkillLifeSpec.h"
#include "LxSkillMovementSpec.h"
#include "LxSkillPropagationSpec.h"
#include "LxSkillSpaceSpec.h"
#include "LxSkillTargetFilterSpec.h"
#include "LxSkillTriggerSpec.h"
#include "LxSkillUnitSpawnSpec.h"
#include "LxSkillUnitSpec.generated.h"

class AActor;

/** 技能单元创建参数，用于描述本次释放时需要覆盖的运行时参数。 */
USTRUCT(BlueprintType, DisplayName="技能单元配置")
struct FLxSkillUnitSpec
{
	GENERATED_BODY()

	/** 具体技能单元Actor类型，形态、视觉、默认运动方式和默认事件语义由该类型自身限定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="技能单元类型")
	ELxSkillUnitType SkillUnitType;

	/** 技能单元创建规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="创建规则")
	FLxSkillUnitSpawnSpec SpawnSpec;

	/** 后期用来进行批量匹配的标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="单元标签")
	FGameplayTag SkillUnitTag;

	/** 本次释放覆盖的空间参数，例如半径、宽度、高度、角度等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	/** 本次释放覆盖的运动参数，例如速度、距离、重力倍率等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="运动参数")
	FLxSkillMovementSpec MovementSpec;

	/** 本次释放覆盖的生命周期参数，例如持续时间、开始延迟和最大触发次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	/** 本次释放覆盖的触发参数，例如周期触发间隔、触发延迟和单目标冷却。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	/** 本次释放覆盖的传播参数，例如穿透、弹跳、连锁或分裂次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="传播参数")
	FLxSkillPropagationSpec PropagationSpec;

	/** 本次释放覆盖的目标筛选参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	/** 本次释放覆盖的命中限制参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};
