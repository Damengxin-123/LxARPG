#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillLifeSpec.generated.h"

/** 技能生命周期参数，描述本次释放需要覆盖的存在时间和触发次数。具体生命周期模式由技能单元Actor类型自身限定。 */
USTRUCT(BlueprintType, DisplayName="技能生命周期参数")
struct FLxSkillLifeSpec
{
	GENERATED_BODY()

	/** 持续时间，生命周期为持续时生效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|生命周期", DisplayName="持续时间")
	float Duration = 0.0f;

	/** 创建后延迟多久才开始执行。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|生命周期", DisplayName="开始前延迟")
	float DelayBeforeStart = 0.0f;

	/** 完成后延迟多久销毁，方便保留表现或等待回调。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|生命周期", DisplayName="销毁前延迟")
	float DelayBeforeDestroy = 0.0f;

	/** 最大触发次数，低于等于 0 表示不限制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|生命周期", DisplayName="最大触发次数")
	int32 MaxTriggerCount = 0;
};
