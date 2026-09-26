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
	/** 标记角色已进入死亡动作，后续分析保持停止状态。 */
	void CompleteCharacterDeath() { bCharacterDeathCompleted = true; }
	/** 更新当前角色位置，供追击距离判断；由控制器在每次分析前调用。 */
	void SetSelfLocation(const FVector& InLocation) { SelfLocation = InLocation; bHasSelfLocation = true; }
	/** 是否正在追击超距后的返回过程中，此时不响应敌人触发的再次追击。 */
	bool IsReturningFromChase() const { return bReturningFromChase; }
	/** 到达巡逻目的地后解除返回保护，允许下一轮重新分析敌情。 */
	void CompleteChaseReturn() { bReturningFromChase = false; }
private:
	/** 当前角色的世界坐标。 */
	FVector SelfLocation = FVector::ZeroVector;
	/** 是否已经收到角色位置，旧的独立分析调用未提供位置时不启用追击限制。 */
	bool bHasSelfLocation = false;
	/** 本轮进入战斗时的起点；切换战斗阶段或受击入口不会重置。 */
	FVector ChaseOrigin = FVector::ZeroVector;
	/** 当前战斗周期是否已记录追击起点。 */
	bool bHasChaseOrigin = false;
	/** 达到追击上限后保持返回，防止感知到原敌人立即重新进入战斗。 */
	bool bReturningFromChase = false;
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
	/** 防止死亡行为完成后再次启动死亡响应。 */
	bool bCharacterDeathCompleted = false;
};
