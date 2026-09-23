#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxARPG/LxSource/Model/AI/LxAIControlAnalysis.h"
#include "LxAIController.generated.h"

class ALxAICharacter;
class ALxBaseCharacter;
class ALxAIRouteActor;
class UAIPerceptionComponent;
class UAISenseConfig_Damage;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
struct FAIStimulus;

/** 当前AI独立维护的单个感知目标记录。 */
struct FLxAITargetMemoryRecord
{
	/** 当前AI直接感知到的目标。 */
	TWeakObjectPtr<ALxBaseCharacter> TargetCharacter;

	/** 最近一次写入该目标时的感知来源。 */
	ELxAIPerceptionSource PerceptionSource = ELxAIPerceptionSource::Unknown;

	/** 最近一次成功感知到目标的世界时间。 */
	double LastSensedTime = 0.0;

	/** 目标是否因直接伤害事件被当前AI标记为敌方。 */
	bool bHostileByDamage = false;

	/** 最近一次视觉成功感知的位置；记忆期不读取目标实时位置。 */
	FVector SightLocation = FVector::ZeroVector;
	/** 最近一次听觉成功感知的位置。 */
	FVector HearingLocation = FVector::ZeroVector;
	/** 最近一次其他来源成功感知的位置。 */
	FVector OtherLocation = FVector::ZeroVector;
	/** 最近一次视觉感知的世界时间。 */
	double SightTime = 0.0;
	/** 最近一次听觉感知的世界时间。 */
	double HearingTime = 0.0;
	/** 最近一次其他来源感知的世界时间。 */
	double OtherTime = 0.0;
	/** 是否仍持有有效视觉记忆。 */
	bool bHasSight = false;
	/** 是否仍持有有效听觉记忆。 */
	bool bHasHearing = false;
	/** 是否仍持有有效其他来源记忆。 */
	bool bHasOther = false;
};

/** AI局势等级或行为发生变化时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAIActionChanged, ELxAISituationLevel, Situation, ELxAIActionType, ActionType);

/** 四入口分析决策变化时广播；具体行为由未来执行器消费。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxAIAnalysisDecisionChanged, FLxAIAnalysisDecision, Decision);

/** 独立完成目标感知、数值对比、行为匹配和执行转发的AI控制器。 */
UCLASS(Blueprintable, DisplayName="AI自动控制器")
class LXARPG_API ALxAIController : public AAIController
{
	GENERATED_BODY()

public:
	/** 创建AI感知组件并注册视觉与伤害感知。 */
	ALxAIController();

	/** 获取当前敌友数值对应的局势等级。 */
	UFUNCTION(BlueprintPure, Category="AI|决策", DisplayName="获取当前AI局势")
	ELxAISituationLevel GetCurrentSituation() const { return CurrentSituation; }

	/** 获取当前AI独立匹配出的行为。 */
	UFUNCTION(BlueprintPure, Category="AI|决策", DisplayName="获取当前AI行为")
	ELxAIActionType GetCurrentAction() const { return CurrentAction; }

	/** 获取当前AI仅根据自身感知生成的战场快照。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="获取当前AI战场快照")
	const FLxAIBattleSnapshot& GetCurrentBattleSnapshot() const { return CurrentBattleSnapshot; }

	/** 获取当前AI私有目标缓存中的记录数量。 */
	UFUNCTION(BlueprintPure, Category="AI|感知", DisplayName="获取AI目标记忆数量")
	int32 GetTargetMemoryCount() const { return TargetMemory.Num(); }

	/** 供范围、交互和效果模块将单向感知结果写入当前AI。 */
	UFUNCTION(BlueprintCallable, Category="AI|感知", DisplayName="报告AI感知目标")
	void ReportPerceivedTarget(AActor* InTargetActor, ELxAIPerceptionSource InPerceptionSource,
		bool bInMarkAsHostile = false);

	/** 有效承伤后立即通知当前角色的四入口分析，不依赖攻击者身份。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", DisplayName="通知AI受到攻击")
	void NotifyReceivedAttack();

	/** 行为消费者完成一次受击响应后调用；本版只输出决策，不执行具体叶行为。 */
	UFUNCTION(BlueprintCallable, Category="AI|分析", DisplayName="完成AI受击响应")
	void CompleteAttackedResponse();

	/** 查询四入口分析的最近决策。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="获取AI分析决策")
	FLxAIAnalysisDecision GetCurrentAnalysisDecision() const;

	/** 四入口分析分支变化事件；具体叶行为执行器尚未接入。 */
	UPROPERTY(BlueprintAssignable, Category="AI|分析", DisplayName="AI分析决策变化")
	FOnLxAIAnalysisDecisionChanged OnAIAnalysisDecisionChanged;

	/** 当前局势等级或行为变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="AI|决策", DisplayName="AI行为变化事件")
	FOnLxAIActionChanged OnAIActionChanged;

protected:
	/** 开始控制AI角色时应用感知配置并启动该角色自己的自动决策。 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 停止控制AI角色时清理该角色私有的目标缓存与决策状态。 */
	virtual void OnUnPossess() override;

	/** 控制器结束运行时清理自动决策定时器。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLxAIAnalysisControllerIntegrationTest;
#endif

	/** 接收视觉和伤害感知，并仅写入当前控制器的目标缓存。 */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* InActor, FAIStimulus InStimulus);

	/** 刷新目标数值、评估局势、匹配行为并执行一次当前行为。 */
	void RunAutomaticDecision();

	/** 用独立感知记忆评估当前角色的四入口分析。 */
	void RunAnalysisDecision();

	/** 执行当前阶段的固定路线逃跑节点，逐点移动并在末点结束响应。 */
	void UpdateRouteFlee(const FLxAIAnalysisDecision& Decision, bool bStartNewBehavior);

	/** 换角色时排空旧刺激并清除引擎感知缓存，避免旧角色视野进入新角色。 */
	void ResetPerceptionForPawnChange();

	/** 记录感知刺激携带的位置，避免声音和失去视觉后偷读目标位置。 */
	void StorePerceivedTarget(AActor* InTargetActor, ELxAIPerceptionSource InSource, const FVector& InLocation, bool bMarkAsHostile);

	/** 根据AI角色配置刷新视觉感知范围和目标记忆寿命。 */
	void ApplyPerceptionConfiguration();

	/** 将仍被视觉持续感知的目标刷新到当前AI私有缓存。 */
	void RefreshActivePerceptionMemory();

	/** 清理无效或超过记忆寿命的当前AI目标记录。 */
	void PruneTargetMemory();

	/** 将当前AI自己的有效感知目标汇总为数值化战场快照。 */
	FLxAIBattleSnapshot BuildBattleSnapshot();

	/** 根据稳定阵营关系和当前AI的直接受击记录计算目标关系。 */
	ELxAITargetRelation ResolveTargetRelation(const ALxBaseCharacter* InTargetCharacter) const;

	/** 根据威胁、自身状态和综合优势值确定局势等级。 */
	ELxAISituationLevel EvaluateSituation(const FLxAIBattleSnapshot& InSnapshot) const;

	/** 按当前局势候选顺序选择第一个通过自身检查且未在本轮排除的行为。 */
	ELxAIActionType SelectFirstExecutableAction(const FLxAIBattleSnapshot& InSnapshot,
		ELxAISituationLevel InSituation, const TSet<ELxAIActionType>& InExcludedActions) const;

	/** 依次尝试当前局势的行为候选，执行失败时排除该行为并立即匹配下一个。 */
	void SelectAndExecuteAction(ELxAISituationLevel InSituation);

	/** 提交已经开始、正在执行或等待条件的行为，并广播最终决策结果。 */
	void ChangeAction(ELxAISituationLevel InSituation, ELxAIActionType InActionType);

	/** 将一方数值与另一方数值转换为-1到1的归一化比较结果。 */
	static float CalculateNormalizedComparison(float InAssistValue, float InEnemyValue);

	/** 获取指定角色当前生命值占上限的状态比例。 */
	static float GetStateRatioForCharacter(const ALxBaseCharacter* InCharacter);

	/** 获取指定角色未乘状态系数前的基础强度。 */
	static float GetBaseStrengthForCharacter(const ALxBaseCharacter* InCharacter);

	/** 获取控制器当前负责的AI角色。 */
	ALxAICharacter* GetAICharacter() const;

	/** AI视觉与伤害感知的统一入口组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="AI感知组件", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	/** 视觉感知参数对象。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="视觉感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** 听觉感知参数对象。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="听觉感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** 伤害感知参数对象，用于接收明确的单向效果来源。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="伤害感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	/** 当前AI独立维护且不会向其他AI广播的目标缓存。 */
	TMap<TWeakObjectPtr<AActor>, FLxAITargetMemoryRecord> TargetMemory;

	/** 当前AI因直接伤害事件而独立标记为敌方的目标。 */
	TSet<TWeakObjectPtr<AActor>> DynamicHostileTargets;

	/** 当前敌友数值对应的局势等级。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|决策", DisplayName="当前AI局势")
	ELxAISituationLevel CurrentSituation = ELxAISituationLevel::NoThreat;

	/** 当前AI从局势候选中匹配出的行为。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|决策", DisplayName="当前AI行为")
	ELxAIActionType CurrentAction = ELxAIActionType::None;

	/** 最近一次根据当前AI私有目标缓存生成的战场快照。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|分析", DisplayName="当前AI战场快照")
	FLxAIBattleSnapshot CurrentBattleSnapshot;

	/** 定时刷新当前AI目标数值与行为匹配的计时器。 */
	FTimerHandle AutomaticDecisionTimer;

	/** 每个控制器独立拥有的分析会话，未选资产时为空。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxAIControlAnalysis> AnalysisSession;

	/** 上次分析运行时刻，保证警觉和响应按真实世界时间推进。 */
	double LastAnalysisTime = 0.0;

	/** 当前固定路线逃跑所属阶段，阶段切换时清除路径进度。 */
	FGuid RouteFleePhaseId;

	/** 当前固定路线逃跑引用的场景路线。 */
	TWeakObjectPtr<ALxAIRouteActor> ActiveFleeRoute;

	/** 下一个需要抵达的样条点索引。 */
	int32 RouteFleePointIndex = 0;

	/** 当前路径点是否已提交移动请求。 */
	bool bRouteFleeMoveRequested = false;

	/** 到达终点后保持完成状态，防止同一阶段反复重启。 */
	bool bRouteFleeCompleted = false;

	/** 当前行为最近一次真正发生切换时的世界时间。 */
	double CurrentActionStartTime = 0.0;
};
