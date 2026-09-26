#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include "LxARPG/LxSource/Model/AI/LxAIControlAnalysis.h"
#include "LxAIController.generated.h"

class ALxAICharacter;
class ALxBaseCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Damage;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class ULxAIBehaviorTreeExecutor;
struct FAIStimulus;

/** 当前 AI 独立维护的单个感知目标记录。 */
struct FLxAITargetMemoryRecord
{
	TWeakObjectPtr<ALxBaseCharacter> TargetCharacter;
	FVector SightLocation = FVector::ZeroVector;
	FVector HearingLocation = FVector::ZeroVector;
	FVector OtherLocation = FVector::ZeroVector;
	double SightTime = 0.0;
	double HearingTime = 0.0;
	double OtherTime = 0.0;
	bool bHasSight = false;
	bool bHasHearing = false;
	bool bHasOther = false;
};

/** 四入口分析决策变化时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxAIAnalysisDecisionChanged, FLxAIAnalysisDecision, Decision);
/** 当前运行叶行为变化时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxAIBehaviorActionChanged, ELxAIBehaviorAction, BehaviorAction);

/** 使用 AI 行为树资产完成感知、分析和叶行为执行的控制器。 */
UCLASS(Blueprintable, DisplayName="AI自动控制器")
class LXARPG_API ALxAIController : public AAIController
{
	GENERATED_BODY()
public:
	/** 创建并配置 AI 感知组件。 */
	ALxAIController();
	/** 获取当前 AI 私有感知记忆数量。 */
	UFUNCTION(BlueprintPure, Category="AI|感知", DisplayName="获取AI目标记忆数量")
	int32 GetTargetMemoryCount() const { return TargetMemory.Num(); }
	/** 将单向感知结果写入当前 AI。 */
	UFUNCTION(BlueprintCallable, Category="AI|感知", DisplayName="报告AI感知目标")
	void ReportPerceivedTarget(AActor* InTargetActor, ELxAIPerceptionSource InPerceptionSource, bool bInMarkAsHostile = false);
	/** 有效承伤后立即触发一次受击分析。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", DisplayName="通知AI受到攻击")
	void NotifyReceivedAttack();
	/** 主动结束当前一次性受击响应。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", DisplayName="完成AI受击响应")
	void CompleteAttackedResponse();
	/** 获取当前分析决策。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="获取AI分析决策")
	FLxAIAnalysisDecision GetCurrentAnalysisDecision() const;
	/** 获取当前叶行为；节点标识无效时表示当前没有叶行为。 */
	UFUNCTION(BlueprintPure, Category="AI|行为树", DisplayName="获取当前行为树行为")
	ELxAIBehaviorAction GetCurrentBehaviorAction() const;
	/** 获取当前叶节点标识。 */
	UFUNCTION(BlueprintPure, Category="AI|行为树", DisplayName="获取当前行为树节点标识")
	FGuid GetCurrentBehaviorActionNodeId() const;

	UPROPERTY(BlueprintAssignable, Category="AI|分析", DisplayName="AI分析决策变化")
	FOnLxAIAnalysisDecisionChanged OnAIAnalysisDecisionChanged;
	UPROPERTY(BlueprintAssignable, Category="AI|行为树", DisplayName="AI行为树行为变化")
	FOnLxAIBehaviorActionChanged OnAIBehaviorActionChanged;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* InActor, FAIStimulus InStimulus);
	void RunAnalysisDecision();
	void ResetPerceptionForPawnChange();
	void StorePerceivedTarget(AActor* InTargetActor, ELxAIPerceptionSource InSource, const FVector& InLocation, bool bMarkAsHostile);
	void ApplyPerceptionConfiguration();
	void RefreshActivePerceptionMemory();
	void PruneTargetMemory();
	ELxAITargetRelation ResolveTargetRelation(const ALxBaseCharacter* InTargetCharacter) const;
	ALxAICharacter* GetAICharacter() const;
	FVector GetLatestRememberedLocation(const FLxAITargetMemoryRecord& InRecord) const;
	/** 将执行器的叶行为变化转发给蓝图监听者。 */
	void HandleLeafChanged(ELxAIBehaviorAction InAction);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="AI感知组件", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="视觉感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="听觉感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="伤害感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
	TMap<TWeakObjectPtr<AActor>, FLxAITargetMemoryRecord> TargetMemory;
	TSet<TWeakObjectPtr<AActor>> DynamicHostileTargets;
	FTimerHandle AutomaticDecisionTimer;
	UPROPERTY(Transient)
	TObjectPtr<ULxAIControlAnalysis> AnalysisSession;
	UPROPERTY(Transient)
	TObjectPtr<ULxAIBehaviorTreeExecutor> BehaviorTreeExecutor;
	double LastAnalysisTime = 0.0;
};
