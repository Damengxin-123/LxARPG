#pragma once

#include "CoreMinimal.h"
#include "LxSkillAuraEffectSpec.generated.h"

class AActor;

/** 光环目标效果被回收的原因。 */
UENUM(BlueprintType, DisplayName="光环目标效果回收原因")
enum class ELxAuraTargetEffectRemoveReason : uint8
{
	LeftRange UMETA(DisplayName="离开光环范围"),
	AuraStopped UMETA(DisplayName="光环主动停止"),
	DurationExpired UMETA(DisplayName="光环持续时间结束"),
	AuraCancelled UMETA(DisplayName="光环异常取消"),
	TargetInvalid UMETA(DisplayName="目标失效")
};

/** 光环效果共有参数。 */
USTRUCT(BlueprintType, DisplayName="光环效果参数")
struct FLxSkillAuraEffectSpec
{
	GENERATED_BODY()

	/** 光环启用后持续的秒数；-1表示一直持续，0和小于-1的值无效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|光环效果", DisplayName="持续时间", meta=(ClampMin="-1.0"))
	float Duration = -1.0f;
};

/** 周期触发型光环独有参数。 */
USTRUCT(BlueprintType, DisplayName="周期光环效果参数")
struct FLxSkillPeriodicAuraEffectSpec
{
	GENERATED_BODY()

	/** 相邻两次范围命中判定之间的时间，单位为秒；必须大于零。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|光环效果|周期", DisplayName="触发间隔", meta=(ClampMin="0.1", UIMin="0.1"))
	float TriggerInterval = 1.0f;

	/** 光环启用后是否立即执行第一次范围命中判定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|光环效果|周期", DisplayName="启用后立即触发")
	bool bTriggerImmediately = true;
};

/** 持续型光环回收单个目标效果时提供的数据。 */
USTRUCT(BlueprintType, DisplayName="光环目标效果回收结果")
struct FLxAuraTargetEffectRemoveResult
{
	GENERATED_BODY()

	/** 需要回收光环效果的目标。 */
	UPROPERTY(BlueprintReadOnly, Category="技能单元|光环效果|结果", DisplayName="效果目标")
	TObjectPtr<AActor> EffectTarget = nullptr;

	/** 本次目标效果被回收的原因。 */
	UPROPERTY(BlueprintReadOnly, Category="技能单元|光环效果|结果", DisplayName="回收原因")
	ELxAuraTargetEffectRemoveReason RemoveReason = ELxAuraTargetEffectRemoveReason::LeftRange;
};
