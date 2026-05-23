#pragma once

#include "CoreMinimal.h"
#include "LxSkillAreaSpec.generated.h"

/** 范围与持续区域的独有参数，通用尺寸、持续时间和触发间隔分别由空间、生命周期和触发结构体提供。 */
USTRUCT(BlueprintType, DisplayName="范围技能形态参数")
struct FLxSkillAreaSpec
{
	GENERATED_BODY()

	/** 范围是否跟随释放者移动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|范围", DisplayName="跟随释放者")
	bool bFollowCaster = false;

	/** 范围是否跟随目标移动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|范围", DisplayName="跟随目标")
	bool bFollowTarget = false;

	/** 是否使用创建瞬间的位置快照，开启后不会继续读取来源位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|范围", DisplayName="使用位置快照")
	bool bUseSnapshotLocation = true;
};
