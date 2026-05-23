#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillPropagationSpec.generated.h"

/** 技能传播参数，描述本次释放需要覆盖的传播次数。具体传播能力由技能单元Actor类型自身支持。 */
USTRUCT(BlueprintType, DisplayName="技能传播参数")
struct FLxSkillPropagationSpec
{
	GENERATED_BODY()

	/** 最大穿透目标数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|穿透", DisplayName="最大穿透数量")
	int32 MaxPierceCount = 0;

	/** 最大弹跳次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|弹跳", DisplayName="最大弹跳次数")
	int32 MaxBounceCount = 0;

	/** 弹跳方向计算方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|弹跳", DisplayName="弹跳方式")
	ELxSkillBounceType BounceType = ELxSkillBounceType::Reflect;

	/** 最大连锁次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|连锁", DisplayName="最大连锁次数")
	int32 MaxChainCount = 0;

	/** 连锁搜索半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|连锁", DisplayName="连锁半径")
	float ChainRadius = 0.0f;

	/** 连锁或弹跳时是否允许重复选择同一个目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|连锁", DisplayName="允许重复目标")
	bool bAllowRepeatTarget = false;

	/** 分裂数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|分裂", DisplayName="分裂数量")
	int32 SplitCount = 0;

	/** 分裂角度范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|分裂", DisplayName="分裂角度")
	float SplitAngle = 0.0f;

	/** 扩散感染半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播|扩散", DisplayName="扩散半径")
	float InfectRadius = 0.0f;
};
