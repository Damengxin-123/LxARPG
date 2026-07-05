#pragma once

#include "CoreMinimal.h"
#include "LxSkillAttachEffectSpec.generated.h"

class AActor;
class UObject;

/** 依附效果结束原因，用于区分正常到期、目标失效和外部终止。 */
UENUM(BlueprintType, DisplayName="依附效果结束原因")
enum class ELxAttachEffectEndReason : uint8
{
	DurationExpired UMETA(DisplayName="持续时间结束"),
	TargetInvalid UMETA(DisplayName="依附目标失效"),
	Stopped UMETA(DisplayName="主动停止"),
	Cancelled UMETA(DisplayName="异常取消")
};

/** 依附效果共有参数，不包含目标；目标必须从前置技能单元命中结果中取得。 */
USTRUCT(BlueprintType, DisplayName="依附效果参数")
struct FLxSkillAttachEffectSpec
{
	GENERATED_BODY()

	/** 依附效果最多持续的秒数；小于等于零表示持续到目标失效或外部终止。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果", DisplayName="持续时间")
	float Duration = 0.0f;

	/** 依附到目标时优先使用的组件插槽；为空时依附到目标根组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果", DisplayName="依附插槽")
	FName AttachSocketName = NAME_None;

	/** 完成吸附后应用到技能单元的相对变换。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果", DisplayName="依附相对变换")
	FTransform RelativeTransform = FTransform::Identity;
};

/** 周期依附效果独有参数。 */
USTRUCT(BlueprintType, DisplayName="周期依附效果参数")
struct FLxSkillPeriodicAttachEffectSpec
{
	GENERATED_BODY()

	/** 相邻两次命中事件之间的时间，单位为秒；必须大于零。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果|周期", DisplayName="触发间隔", meta=(ClampMin="0.1", UIMin="0.1"))
	float TriggerInterval = 1.0f;

	/** 完成依附后是否立即触发第一次命中。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果|周期", DisplayName="依附后立即触发")
	bool bTriggerImmediately = true;
};

/** 依附效果结束数据，供效果层准确撤销本技能单元施加的持续效果。 */
USTRUCT(BlueprintType, DisplayName="依附效果结束结果")
struct FLxAttachEffectEndResult
{
	GENERATED_BODY()

	/** 本次依附的目标。 */
	UPROPERTY(BlueprintReadOnly, Category="技能单元|依附效果|结果", DisplayName="依附目标")
	TObjectPtr<AActor> AttachTarget = nullptr;

	/** 提供命中目标的前置技能单元。 */
	UPROPERTY(BlueprintReadOnly, Category="技能单元|依附效果|结果", DisplayName="前置技能单元")
	TObjectPtr<UObject> PreviousSkillUnit = nullptr;

	/** 本次依附效果结束的原因。 */
	UPROPERTY(BlueprintReadOnly, Category="技能单元|依附效果|结果", DisplayName="结束原因")
	ELxAttachEffectEndReason EndReason = ELxAttachEffectEndReason::DurationExpired;

	/** 是否曾经成功广播过命中事件。 */
	UPROPERTY(BlueprintReadOnly, Category="技能单元|依附效果|结果", DisplayName="已经生效")
	bool bEffectApplied = false;
};
