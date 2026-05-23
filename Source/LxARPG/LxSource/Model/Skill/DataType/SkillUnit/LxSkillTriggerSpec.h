#pragma once

#include "CoreMinimal.h"
#include "LxSkillTriggerSpec.generated.h"

/** 技能触发参数，描述本次释放需要覆盖的触发节奏。具体触发事件由技能单元Actor类型自身发布。 */
USTRUCT(BlueprintType, DisplayName="技能触发参数")
struct FLxSkillTriggerSpec
{
	GENERATED_BODY()

	/** 周期触发间隔，触发类型为周期触发时生效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="触发间隔")
	float TickInterval = 0.0f;

	/** 满足触发条件后延迟多久真正触发。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="触发延迟")
	float TriggerDelay = 0.0f;

	/** 创建后是否立即触发一次。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="立即触发")
	bool bTriggerImmediately = true;

	/** 是否每个目标只触发一次。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="每目标只触发一次")
	bool bTriggerOncePerTarget = false;

	/** 同一个目标两次触发之间的冷却时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="单目标触发冷却")
	float PerTargetCooldown = 0.0f;
};
