#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillMeleeSpec.generated.h"

/** 近战技能独有参数，角度、半径和最大目标数由通用空间与目标筛选结构提供。 */
USTRUCT(BlueprintType, DisplayName="近战技能形态参数")
struct FLxSkillMeleeSpec
{
	GENERATED_BODY()

	/** 近战判定来源，例如角色朝向、武器插槽、武器轨迹或动画通知。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|近战", DisplayName="近战判定来源")
	ELxSkillMeleeSourceType MeleeSourceType = ELxSkillMeleeSourceType::CasterForward;

	/** 是否使用武器轨迹进行判定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|近战", DisplayName="使用武器轨迹")
	bool bUseWeaponTrajectory = false;

	/** 近战判定使用的插槽名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|近战", DisplayName="武器插槽名称")
	FName WeaponSocketName = NAME_None;
};
