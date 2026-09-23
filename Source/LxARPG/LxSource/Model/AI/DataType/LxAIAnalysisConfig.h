#pragma once

#include "CoreMinimal.h"
#include "LxAIAnalysisConfig.generated.h"

/** 分析能力固定产生的行为入口；前三项是持续条件，受击是一次响应事件。 */
UENUM(BlueprintType, meta=(DisplayName="AI分析入口"))
enum class ELxAIBehaviorEntry : uint8
{
	Calm UMETA(DisplayName="平静状态"),
	EnemyFound UMETA(DisplayName="发现敌人"),
	EnemyNear UMETA(DisplayName="敌人靠近"),
	Attacked UMETA(DisplayName="受到攻击")
};

/** 按角色类型共享的局势分析参数，不包含角色当前局势。 */
USTRUCT(BlueprintType, meta=(DisplayName="AI分析能力配置"))
struct LXARPG_API FLxAIAnalysisConfig
{
	GENERATED_BODY()

	/** 敌人距离不大于此值时进入靠近条件，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="距离分析", meta=(DisplayName="靠近进入距离", ClampMin="0.0", Units="m"))
	float NearEnterDistanceMeters = 5.0f;

	/** 敌人距离大于此值时退出靠近条件；大于进入距离以避免边界抖动。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="距离分析", meta=(DisplayName="靠近退出距离", ClampMin="0.01", Units="m"))
	float NearExitDistanceMeters = 7.0f;

	/** 每次受击刷新警觉时间；警觉期间没有已知敌人也不会进入平静。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="受击分析", meta=(DisplayName="受击警觉时间", ClampMin="0.0", Units="s"))
	float AttackedAlertSeconds = 5.0f;

	/** 检查距离滞回和警觉时长；拒绝非有限值。 */
	bool ValidateConfiguration(FText& OutError) const;
};
