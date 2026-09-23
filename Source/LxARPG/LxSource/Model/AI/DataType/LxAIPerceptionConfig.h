#pragma once

#include "CoreMinimal.h"
#include "LxAIPerceptionConfig.generated.h"

/** 角色类型共用的感知能力参数；不保存目标、刺激或计时等角色实例状态。 */
USTRUCT(BlueprintType, meta=(DisplayName="AI感知能力配置"))
struct LXARPG_API FLxAIPerceptionConfig
{
	GENERATED_BODY()

	/** 是否允许通过视觉发现目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="视觉", meta=(DisplayName="启用视觉"))
	bool bEnableSight = true;

	/** 首次发现目标的最大距离，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="视觉", meta=(DisplayName="发现距离", ClampMin="0.01", Units="m", EditCondition="bEnableSight"))
	float SightRadiusMeters = 30.0f;

	/** 已发现目标的视觉保持距离，不能小于发现距离，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="视觉", meta=(DisplayName="丢失目标距离", ClampMin="0.01", Units="m", EditCondition="bEnableSight"))
	float LoseSightRadiusMeters = 35.0f;

	/** 相对于朝向的视野半角；60度对应总计120度视野。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="视觉", meta=(DisplayName="视野半角", ClampMin="0.0", ClampMax="180.0", Units="deg", EditCondition="bEnableSight"))
	float SightHalfAngleDegrees = 60.0f;

	/** 视觉刺激的记忆时间；0表示不因超时遗忘，不表示持续看得到目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="视觉", meta=(DisplayName="目标记忆时间", ClampMin="0.0", Units="s", EditCondition="bEnableSight"))
	float SightMemorySeconds = 5.0f;

	/** 是否允许通过声音感知目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="听觉", meta=(DisplayName="启用听觉"))
	bool bEnableHearing = true;

	/** 听觉范围，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="听觉", meta=(DisplayName="听觉距离", ClampMin="0.01", Units="m", EditCondition="bEnableHearing"))
	float HearingRadiusMeters = 20.0f;

	/** 听觉刺激的记忆时间；0表示不因超时遗忘。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="听觉", meta=(DisplayName="声音记忆时间", ClampMin="0.0", Units="s", EditCondition="bEnableHearing"))
	float HearingMemorySeconds = 3.0f;

	/** 视觉与听觉共用的敌对目标筛选；敌我关系由项目阵营规则判断。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="目标筛选", meta=(DisplayName="识别敌对目标"))
	bool bDetectEnemies = true;

	/** 是否识别阵营规则判定为中立的目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="目标筛选", meta=(DisplayName="识别中立目标"))
	bool bDetectNeutrals = false;

	/** 是否识别阵营规则判定为友方的目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="目标筛选", meta=(DisplayName="识别友方目标"))
	bool bDetectFriendlies = false;

	/** 校验已启用能力的参数；关闭能力保留其配置，所有目标筛选关闭也是合法选择。 */
	bool ValidateConfiguration(FText& OutError) const;
};
