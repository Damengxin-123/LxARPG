#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DataType/LxAIAnalysisConfig.h"
#include "LxAIControlAnalysis.generated.h"

class AActor;
class ULxAIBehaviorTreeAsset;

/** 一次分析得出的入口、状态和阶段；执行进度由角色行为执行器独立维护。 */
USTRUCT(BlueprintType, meta=(DisplayName="AI分析决策"))
struct LXARPG_API FLxAIAnalysisDecision
{
	GENERATED_BODY()

	/** 是否存在条件符合的分支。 */
	UPROPERTY(BlueprintReadOnly, Category="AI分析", meta=(DisplayName="存在有效分支"))
	bool bHasBranch = false;

	/** 本次命中的分析入口。 */
	UPROPERTY(BlueprintReadOnly, Category="AI分析", meta=(DisplayName="入口类型"))
	ELxAIBehaviorEntry Entry = ELxAIBehaviorEntry::Calm;

	/** 当前入口标识。 */
	UPROPERTY(BlueprintReadOnly, Category="AI分析", meta=(DisplayName="入口标识"))
	FGuid EntryId;

	/** 当前状态标识；不同入口可以返回同一个状态。 */
	UPROPERTY(BlueprintReadOnly, Category="AI分析", meta=(DisplayName="状态标识"))
	FGuid StateId;

	/** 当前阶段标识。 */
	UPROPERTY(BlueprintReadOnly, Category="AI分析", meta=(DisplayName="阶段标识"))
	FGuid PhaseId;

	/** 仅分支发生变化或新响应需要启动时为真；同一响应不会每次受击重启。 */
	UPROPERTY(BlueprintReadOnly, Category="AI分析", meta=(DisplayName="需要开始新行为"))
	bool bStartNewBehavior = false;
};

/** 每个角色独立拥有的分析会话；消费感知事实与受击事件，绝不写入共享配置。 */
UCLASS(BlueprintType, meta=(DisplayName="AI控制分析会话"))
class LXARPG_API ULxAIControlAnalysis : public UObject
{
	GENERATED_BODY()
public:
	/** 绑定角色类型配置并清空本角色的旧局势、受击事件与决策。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", meta=(DisplayName="初始化分析会话"))
	void Initialize(ULxAIBehaviorTreeAsset* InConfiguration);

	/** 输入有效受击；连续受击刷新警觉时间，但不排队或重启尚未完成的响应。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", meta=(DisplayName="通知受到攻击"))
	void NotifyAttacked();

	/** 由响应消费者在阶段完成或主动放弃时调用，解除一次受击响应。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", meta=(DisplayName="完成受击响应"))
	void CompleteAttackedResponse();

	/** 消费已知敌人及其最后已知距离；无敌人传空。距离为米、时间为秒、生命为0～1。
	 * 不可打断时保持当前分支；受击脉冲不会跨帧排队，警觉时间仍正常推进。
	 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", meta=(DisplayName="更新局势分析", AdvancedDisplay="bCanInterrupt"))
	FLxAIAnalysisDecision Evaluate(AActor* KnownEnemy, float KnownEnemyDistanceMeters, float HealthRatio, float DeltaSeconds, bool bCanInterrupt = true);

	/** 获取本角色最新决策，不能作为其他角色的运行状态。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", meta=(DisplayName="获取当前分析决策"))
	FLxAIAnalysisDecision GetCurrentDecision() const { return CurrentDecision; }

	/** 当前是否处于受击后的警觉期。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", meta=(DisplayName="是否处于受击警觉期"))
	bool IsAlertAfterAttack() const { return AlertSecondsRemaining > 0.0f; }

	/** 当前是否保留一个等待完成确认的受击响应。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", meta=(DisplayName="受击响应是否进行中"))
	bool IsAttackedResponseActive() const { return bAttackedResponseActive; }
private:
	/** 共享配置引用，分析会话只读。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxAIBehaviorTreeAsset> Configuration;
	/** 当前选择的敌人弱引用，用于目标改变时重置距离滞回。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousEnemy;
	/** 最新分支选择。 */
	UPROPERTY(Transient)
	FLxAIAnalysisDecision CurrentDecision;
	/** 本角色剩余警觉时间。 */
	float AlertSecondsRemaining = 0.0f;
	/** 下一次分析消费的受击脉冲。 */
	bool bPendingAttack = false;
	/** 等待响应消费者完成的一次受击。 */
	bool bAttackedResponseActive = false;
	/** 进入退出距离之间保持原值的滞回状态。 */
	bool bEnemyNear = false;
	/** 前次响应已完成；下一分支即使相同也应重新开始。 */
	bool bResponseCompleted = false;
};
