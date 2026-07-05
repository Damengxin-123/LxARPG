#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillRaySpec.generated.h"

/** 射线公共命中规则，不包含由蓝图碰撞体决定的检测范围和半径。 */
USTRUCT(BlueprintType, DisplayName="射线公共参数")
struct FLxSkillRaySpec
{
	GENERATED_BODY()

	/** 明确指定的射线方向；零向量表示改用释放者指向目标的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线", DisplayName="射线方向")
	FVector RayDirection = FVector::ZeroVector;

	/** 使用前置命中结果创建单次射线时的方向选择；保持来源朝向时沿用射线自身方向规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;

	/** 是否允许射线命中多个目标；关闭时只保留距离射线起点最近的有效目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线", DisplayName="是否穿过目标")
	bool bPassThroughTargets = true;

	/** 是否忽略目标与射线起点之间阻挡可见性通道的场景障碍物。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线", DisplayName="是否穿过障碍物")
	bool bPassThroughObstacles = true;
};

/** 单次射线效果参数，控制同批射线的数量和横向间距。 */
USTRUCT(BlueprintType, DisplayName="单次射线效果参数")
struct FLxSingleRayEffectSpec
{
	GENERATED_BODY()

	/** 同一次释放创建的射线技能单元数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线|单次", DisplayName="发射数量", meta=(ClampMin="1", UIMin="1"))
	int32 LaunchCount = 1;

	/** 多条平行射线沿局部右方向居中排列时的相邻间距，单位 m。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线|单次", DisplayName="射线间距（m）", meta=(ClampMin="0.0", UIMin="0.0"))
	float RaySpacing = 0.0f;

	/** 完成单次命中判定后继续保留射线表现的时间，单位秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线|单次", DisplayName="射线表现保留时间（秒）",
		meta=(ClampMin="0.01", UIMin="0.01"))
	float VisualRetentionDuration = 0.2f;

	/** 获取 UE 内部使用的射线间距，单位 cm。 */
	float GetRaySpacingInUnrealUnits() const { return FMath::Max(RaySpacing, 0.0f) * 100.0f; }
};

/** 持续射线效果参数，控制运行期间重新执行命中检测的周期。 */
USTRUCT(BlueprintType, DisplayName="持续射线效果参数")
struct FLxContinuousRayEffectSpec
{
	GENERATED_BODY()

	/** 启用期间重新进行一次射线命中判定的时间间隔，单位秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线|持续", DisplayName="触发周期（秒）", meta=(ClampMin="0.01", UIMin="0.01"))
	float TriggerInterval = 0.2f;
};
