#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxAIController.generated.h"

class ALxAICharacter;
class ALxBaseCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Damage;
class UAISenseConfig_Sight;
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
};

/** AI局势等级或行为发生变化时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAIActionChanged, ELxAISituationLevel, Situation, ELxAIActionType, ActionType);

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
	/** 接收视觉和伤害感知，并仅写入当前控制器的目标缓存。 */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* InActor, FAIStimulus InStimulus);

	/** 刷新目标数值、评估局势、匹配行为并执行一次当前行为。 */
	void RunAutomaticDecision();

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

	/** 当前行为最近一次真正发生切换时的世界时间。 */
	double CurrentActionStartTime = 0.0;
};
