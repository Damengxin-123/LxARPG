#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillAttachSpec.generated.h"

/** 附着技能独有参数，持续时间和周期触发由生命周期与触发结构提供。 */
USTRUCT(BlueprintType, DisplayName="附着技能形态参数")
struct FLxSkillAttachSpec
{
	GENERATED_BODY()

	/** 附着对象类型，例如释放者、命中目标、选中目标或生成实体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|附着", DisplayName="附着对象类型")
	ELxSkillAttachUnitTargetType AttachTargetType = ELxSkillAttachUnitTargetType::HitTarget;

	/** 附着插槽名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|附着", DisplayName="附着插槽名称")
	FName AttachSocketName = NAME_None;

	/** 目标死亡时是否移除附着单元。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|附着", DisplayName="目标死亡移除")
	bool bRemoveWhenTargetDead = true;

	/** 被驱散时是否移除附着单元。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|附着", DisplayName="可被驱散移除")
	bool bRemoveWhenDispelled = true;
};
