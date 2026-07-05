#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillAreaSpec.h"
#include "LxSkillAttachEffectSpec.h"
#include "LxSkillAuraEffectSpec.h"
#include "LxSkillHitLimitSpec.h"
#include "LxSkillLifeSpec.h"
#include "LxSkillGroundBounceProjectileSpec.h"
#include "LxSkillLobProjectileSpec.h"
#include "LxSkillMeleeSpec.h"
#include "LxSkillMovementSpec.h"
#include "LxSkillProjectileSpec.h"
#include "LxSkillPropagationSpec.h"
#include "LxSkillRaySpec.h"
#include "LxSkillSpawnEntitySpec.h"
#include "LxSkillSpaceSpec.h"
#include "LxSkillTargetFilterSpec.h"
#include "LxSkillTriggerSpec.h"
#include "LxSkillTriggerUnitSpec.h"
#include "LxSkillUnitCreateParams.generated.h"

class AActor;

/** 投射物技能单元创建参数，只暴露投射物自身需要的配置。 */
USTRUCT(BlueprintType, DisplayName="投射物技能单元创建参数")
struct FLxProjectileSkillUnitCreateParams
{
	GENERATED_BODY()

	/** 投射物运行参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="投射物参数")
	FLxSkillProjectileSpec ProjectileSpec;

	/** 使用前置命中结果创建投射物时采用的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;
};

/** 地面弹跳投射物技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="地面弹跳投射物技能单元创建参数")
struct FLxGroundBounceProjectileSkillUnitCreateParams
{
	GENERATED_BODY()

	/** 投射物共有运行参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|地面弹跳投射物", DisplayName="投射物参数")
	FLxSkillProjectileSpec ProjectileSpec;

	/** 地面弹跳投射物专用轨迹参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|地面弹跳投射物", DisplayName="地面弹跳参数")
	FLxSkillGroundBounceProjectileSpec GroundBounceSpec;

	/** 使用前置命中结果创建投射物时采用的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|地面弹跳投射物", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;
};

/** 抛射投射物技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="抛射投射物技能单元创建参数")
struct FLxLobProjectileSkillUnitCreateParams
{
	GENERATED_BODY()

	/** 投射物共有运行参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|抛射投射物", DisplayName="投射物参数")
	FLxSkillProjectileSpec ProjectileSpec;

	/** 抛射投射物专用轨迹参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|抛射投射物", DisplayName="抛射参数")
	FLxSkillLobProjectileSpec LobSpec;

	/** 使用前置命中结果创建投射物时采用的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|抛射投射物", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;
};

/** 直接命中型范围效果创建参数。 */
USTRUCT(BlueprintType, DisplayName="直接命中型范围效果创建参数")
struct FLxDirectHitAreaEffectCreateParams
{
	GENERATED_BODY()

	/** 范围效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="范围效果参数")
	FLxSkillAreaEffectSpec AreaEffectSpec;

	/** 使用前置命中结果创建范围效果时采用的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;
};

/** 持续型范围效果创建参数。 */
USTRUCT(BlueprintType, DisplayName="持续型范围效果创建参数")
struct FLxDurationAreaEffectCreateParams
{
	GENERATED_BODY()

	/** 范围效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="范围效果参数")
	FLxSkillAreaEffectSpec AreaEffectSpec;

	/** 持续型范围效果参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="持续型参数")
	FLxSkillDurationAreaEffectSpec DurationAreaEffectSpec;

	/** 使用前置命中结果创建范围效果时采用的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;
};

/** 缩放型范围效果创建参数。 */
USTRUCT(BlueprintType, DisplayName="缩放型范围效果创建参数")
struct FLxScalingAreaEffectCreateParams
{
	GENERATED_BODY()

	/** 范围效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="范围效果参数")
	FLxSkillAreaEffectSpec AreaEffectSpec;

	/** 缩放型范围效果参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="缩放型参数")
	FLxSkillScalingAreaEffectSpec ScalingAreaEffectSpec;

	/** 使用前置命中结果创建范围效果时采用的方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|范围效果", DisplayName="前置结果方向选择")
	ELxSkillResultDirectionType ResultDirectionType = ELxSkillResultDirectionType::KeepSourceRotation;
};
/** 近战效果技能单元创建参数，只暴露近战效果单元自身需要的配置。 */
USTRUCT(BlueprintType, DisplayName="近战效果技能单元创建参数")
struct FLxMeleeSkillUnitCreateParams
{
	GENERATED_BODY()

	/** 近战效果单元运行参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|近战", DisplayName="近战效果参数")
	FLxSkillMeleeSpec MeleeSpec;
};
/** 单次射线效果单元创建参数，只包含公共射线限制和批量发射参数。 */
USTRUCT(BlueprintType, DisplayName="单次射线效果单元创建参数")
struct FLxSingleRayEffectCreateParams
{
	GENERATED_BODY()

	/** 射线穿过目标和障碍物的公共规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|射线", DisplayName="射线公共参数")
	FLxSkillRaySpec RaySpec;

	/** 同批射线数量和横向间距。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|射线", DisplayName="单次射线参数")
	FLxSingleRayEffectSpec SingleRaySpec;
};

/** 持续射线效果单元创建参数，只包含公共射线限制和周期检测参数。 */
USTRUCT(BlueprintType, DisplayName="持续射线效果单元创建参数")
struct FLxContinuousRayEffectCreateParams
{
	GENERATED_BODY()

	/** 射线穿过目标和障碍物的公共规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|射线", DisplayName="射线公共参数")
	FLxSkillRaySpec RaySpec;

	/** 持续射线周期检测参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|射线", DisplayName="持续射线参数")
	FLxContinuousRayEffectSpec ContinuousRaySpec;
};

/** 生成实体类技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="生成实体技能单元创建参数")
struct FLxSpawnEntitySkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生成实体参数")
	FLxSkillSpawnEntitySpec SpawnEntitySpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;
};

/** 触发器技能单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="触发器技能单元创建参数")
struct FLxTriggerSkillUnitCreateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="空间参数")
	FLxSkillSpaceSpec SpaceSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发器参数")
	FLxSkillTriggerUnitSpec TriggerUnitSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="生命周期参数")
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="触发参数")
	FLxSkillTriggerSpec TriggerSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="目标筛选参数")
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="命中限制参数")
	FLxSkillHitLimitSpec HitLimitSpec;
};

/** 持续生效依附效果单元创建参数；依附目标由创建函数传入的前置命中结果提供。 */
USTRUCT(BlueprintType, DisplayName="持续生效依附效果创建参数")
struct FLxContinuousAttachEffectCreateParams
{
	GENERATED_BODY()

	/** 依附效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|依附效果", DisplayName="依附效果参数")
	FLxSkillAttachEffectSpec AttachEffectSpec;
};

/** 周期触发依附效果单元创建参数；依附目标由创建函数传入的前置命中结果提供。 */
USTRUCT(BlueprintType, DisplayName="周期触发依附效果创建参数")
struct FLxPeriodicAttachEffectCreateParams
{
	GENERATED_BODY()

	/** 依附效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|依附效果", DisplayName="依附效果参数")
	FLxSkillAttachEffectSpec AttachEffectSpec;

	/** 周期触发独有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|依附效果", DisplayName="周期依附效果参数")
	FLxSkillPeriodicAttachEffectSpec PeriodicSpec;
};

/** 持续型光环效果单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="持续型光环效果创建参数")
struct FLxContinuousAuraEffectCreateParams
{
	GENERATED_BODY()

	/** 光环效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|光环效果", DisplayName="光环效果参数")
	FLxSkillAuraEffectSpec AuraEffectSpec;
};

/** 周期触发型光环效果单元创建参数。 */
USTRUCT(BlueprintType, DisplayName="周期触发型光环效果创建参数")
struct FLxPeriodicAuraEffectCreateParams
{
	GENERATED_BODY()

	/** 光环效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|光环效果", DisplayName="光环效果参数")
	FLxSkillAuraEffectSpec AuraEffectSpec;

	/** 周期触发型光环独有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建|光环效果", DisplayName="周期光环参数")
	FLxSkillPeriodicAuraEffectSpec PeriodicSpec;
};
