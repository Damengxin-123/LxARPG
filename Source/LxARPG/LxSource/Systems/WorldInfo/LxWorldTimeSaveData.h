#pragma once
#include "CoreMinimal.h"
#include "LxWorldTimeSaveData.generated.h"

/** 地图共享时间快照；旧档缺少字段时从第一天早晨八点开始。 */
USTRUCT(BlueprintType, meta=(DisplayName="世界时间存档"))
struct LXARPG_API FLxWorldTimeSaveData
{
	GENERATED_BODY()
	/** 从第一天零点累计的游戏秒数，不随跨天归零。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly, Category="世界信息|时间", meta=(DisplayName="累计游戏秒数", ClampMin="0"))
	double TotalGameSeconds = 28800.0;
	/** 每个游戏运行秒推进的历法秒数，默认二十四分钟经过一天。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly, Category="世界信息|时间", meta=(DisplayName="时间流速", ClampMin="0"))
	double TimeScale = 60.0;
	/** 检查非有限值和负数，阻止无效时间参与计算。 */
	bool IsValid() const
	{
		return FMath::IsFinite(TotalGameSeconds) && TotalGameSeconds >= 0.0
			&& FMath::IsFinite(TimeScale) && TimeScale >= 0.0;
	}
};
