#pragma once

#include "CoreMinimal.h"
#include "LxTrainingDummyDamageRecord.generated.h"

/** 木桩的一次最终承伤记录，列表按最近受击在前排列。 */
USTRUCT(BlueprintType, DisplayName="木桩伤害记录", meta=(Category="木桩|伤害记录"))
struct LXARPG_API FLxTrainingDummyDamageRecord
{
	GENERATED_BODY()

	/** 本次攻击的连续序号，用于区分相同伤害数值的多次攻击。 */
	UPROPERTY(BlueprintReadOnly, Category="木桩|伤害记录", DisplayName="攻击序号")
	int32 AttackNumber = 0;

	/** 本次实际扣除的护盾和生命值合计伤害。 */
	UPROPERTY(BlueprintReadOnly, Category="木桩|伤害记录", DisplayName="实际伤害")
	float DamageValue = 0.0f;

	/** 本次攻击是否触发暴击。 */
	UPROPERTY(BlueprintReadOnly, Category="木桩|伤害记录", DisplayName="是否暴击")
	bool bCriticalHit = false;

	/** 收到攻击时的游戏时间，单位为秒，保留两位小数。 */
	UPROPERTY(BlueprintReadOnly, Category="木桩|伤害记录", DisplayName="受击时间（秒）", meta=(Units="s"))
	float ReceivedTimeSeconds = 0.0f;
};
