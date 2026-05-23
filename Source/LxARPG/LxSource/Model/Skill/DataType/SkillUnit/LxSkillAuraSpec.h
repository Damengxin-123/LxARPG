#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillAuraSpec.generated.h"

/** 光环技能独有参数，范围、目标阵营、持续时间和周期触发由通用结构体提供。 */
USTRUCT(BlueprintType, DisplayName="光环技能形态参数")
struct FLxSkillAuraSpec
{
	GENERATED_BODY()

	/** 光环中心对象类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|光环", DisplayName="光环拥有者类型")
	ELxSkillAuraOwnerType AuraOwnerType = ELxSkillAuraOwnerType::Caster;

	/** 是否作为开关型光环使用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|光环", DisplayName="开关型光环")
	bool bToggleAura = false;

	/** 再次释放时是否移除已有光环。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|光环", DisplayName="再次释放移除")
	bool bRemoveOnRecast = false;
};
