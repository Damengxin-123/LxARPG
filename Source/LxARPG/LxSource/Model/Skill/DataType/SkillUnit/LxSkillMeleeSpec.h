#pragma once

#include "CoreMinimal.h"
#include "LxSkillMeleeSpec.generated.h"

class AActor;

/** 近战效果单元命中上下文，只携带技能对外需要的单次命中数据。 */
USTRUCT(BlueprintType, DisplayName="近战效果单元命中上下文")
struct FLxMeleeHitContext
{
	GENERATED_BODY()

	/** 本次武器行为命中的有效目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|近战|命中", DisplayName="命中目标")
	TObjectPtr<AActor> HitTarget = nullptr;

	/** 本次武器行为命中目标时的世界位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|近战|命中", DisplayName="命中位置")
	FVector HitLocation = FVector::ZeroVector;
};

/** 近战效果单元参数，只限制单次释放能够接受的有效武器命中次数。 */
USTRUCT(BlueprintType, DisplayName="近战效果单元参数")
struct FLxSkillMeleeSpec
{
	GENERATED_BODY()

	/**
	 * 单次释放最多能够接受的有效武器命中次数。
	 * 0 表示不限制，所有由武器判定为有效的命中都会触发技能命中。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|近战|限制", DisplayName="最大命中次数", meta=(ClampMin="0"))
	int32 MaxHitCount = 0;

	/** 是否允许同一次近战攻击重复命中同一个目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|近战|限制", DisplayName="允许重复命中相同目标")
	bool bAllowRepeatedHitSameTarget = false;
};
