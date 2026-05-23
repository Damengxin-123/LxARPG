#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillTriggerUnitSpec.generated.h"

/** 触发器技能独有参数，触发范围和持续时间由空间与生命周期结构提供。 */
USTRUCT(BlueprintType, DisplayName="触发器技能形态参数")
struct FLxSkillTriggerUnitSpec
{
	GENERATED_BODY()

	/** 触发器条件，例如目标进入、目标停留、手动引爆或时间结束。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|触发器", DisplayName="触发条件")
	ELxSkillTriggerConditionType TriggerConditionType = ELxSkillTriggerConditionType::TargetEnter;

	/** 触发后是否销毁触发器。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|触发器", DisplayName="触发后销毁")
	bool bDestroyAfterTriggered = true;
};
