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

/** AI自动决策出的战略或行为发生变化时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAIActionChanged, ELxAITacticalStrategy, Strategy, ELxAIActionType, ActionType);

/** 自动完成感知共享、群体态势分析、配置评分和通用行为执行的AI控制器。 */
UCLASS(Blueprintable, DisplayName="AI自动控制器")
class LXARPG_API ALxAIController : public AAIController
{
	GENERATED_BODY()

public:
	/** 创建AI感知组件并注册视觉与伤害感知。 */
	ALxAIController();

	/** 获取当前自动决策选出的战略。 */
	UFUNCTION(BlueprintPure, Category="AI|决策", DisplayName="获取当前AI战略")
	ELxAITacticalStrategy GetCurrentStrategy() const { return CurrentStrategy; }

	/** 获取当前自动决策选出的个人行为。 */
	UFUNCTION(BlueprintPure, Category="AI|决策", DisplayName="获取当前AI行为")
	ELxAIActionType GetCurrentAction() const { return CurrentAction; }

	/** 获取最近一次群体共享情报生成的战场快照。 */
	UFUNCTION(BlueprintPure, Category="AI|分析", DisplayName="获取当前AI战场快照")
	const FLxAIBattleSnapshot& GetCurrentBattleSnapshot() const { return CurrentBattleSnapshot; }

	/** 当前战略或行为变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="AI|决策", DisplayName="AI行为变化事件")
	FOnLxAIActionChanged OnAIActionChanged;

protected:
	/** 开始控制AI角色时注册群体、应用感知配置并启动自动决策。 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 停止控制AI角色时注销群体并释放行为占用。 */
	virtual void OnUnPossess() override;

	/** 控制器结束运行时清理自动决策定时器。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 接收视觉、伤害等感知结果并写入群体共享情报。 */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* InActor, FAIStimulus InStimulus);

	/** 执行一次完整的共享情报分析、战略判断、行为评分和行为执行。 */
	void RunAutomaticDecision();

	/** 根据AI角色配置刷新视觉感知范围和情报寿命。 */
	void ApplyPerceptionConfiguration();

	/** 将群体共享目标压缩为固定大小的战场快照。 */
	FLxAIBattleSnapshot BuildBattleSnapshot() const;

	/** 根据目标阵营及动态伤害行为计算当前关系。 */
	ELxAITargetRelation ResolveTargetRelation(const ALxBaseCharacter* InTargetCharacter) const;

	/** 根据战场威胁、优势值和自身生命状态选择战略。 */
	ELxAITacticalStrategy EvaluateStrategy(const FLxAIBattleSnapshot& InSnapshot) const;

	/** 从全部配置规则中过滤并选择评分最高的个人行为。 */
	ELxAIActionType SelectBestAction(const FLxAIBattleSnapshot& InSnapshot, ELxAITacticalStrategy InStrategy,
		float& OutBestScore) const;

	/** 判断指定行为规则是否满足当前硬限制。 */
	bool IsActionRuleAvailable(const FLxAIActionRule& InRule, const FLxAIBattleSnapshot& InSnapshot,
		ELxAITacticalStrategy InStrategy) const;

	/** 计算一个已经通过限制检查的行为规则得分。 */
	float CalculateActionScore(const FLxAIActionRule& InRule, const FLxAIBattleSnapshot& InSnapshot) const;

	/** 切换当前行为并同步群体行为占用。 */
	void ChangeAction(ELxAITacticalStrategy InStrategy, ELxAIActionType InActionType, float InActionScore);

	/** 使用内置通用执行器自动执行当前行为。 */
	void ExecuteCurrentAction();

	/** 执行无威胁巡逻行为。 */
	void ExecutePatrolAction();

	/** 执行警戒行为并关注当前最高威胁目标。 */
	void ExecuteAlertAction();

	/** 接近最高威胁目标并按配置自动释放攻击技能。 */
	void ExecuteAttackAction();

	/** 靠近协助方中心并面向最高威胁目标进行协助防御。 */
	void ExecuteDefendAction();

	/** 接近最低状态友方并按配置自动释放治疗技能。 */
	void ExecuteHealAction();

	/** 选择远离敌方中心的可达位置并撤退。 */
	void ExecuteRetreatAction();

	/** 获取指定基础角色的当前生命比例。 */
	static float GetHealthRatioForCharacter(const ALxBaseCharacter* InCharacter);

	/** 获取指定基础角色参与群体分析的有效战力。 */
	static float GetCombatPowerForCharacter(const ALxBaseCharacter* InCharacter);

	/** 获取控制器当前负责的AI角色。 */
	ALxAICharacter* GetAICharacter() const;

	/** 为未配置群体ID的角色生成只包含自己的运行时群体ID。 */
	FName ResolveRuntimeGroupId(const ALxAICharacter* InCharacter) const;

	/** AI视觉与伤害感知的统一入口组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="AI感知组件", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	/** 视觉感知参数对象。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="视觉感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** 伤害感知参数对象，用于把产生敌对行为的中立目标加入动态威胁。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|感知", DisplayName="伤害感知配置", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	/** 当前运行时使用的群体ID。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|群体", DisplayName="运行时群体ID")
	FName RuntimeGroupId = NAME_None;

	/** 当前自动决策选出的战略。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|决策", DisplayName="当前AI战略")
	ELxAITacticalStrategy CurrentStrategy = ELxAITacticalStrategy::Idle;

	/** 当前自动决策选出的个人行为。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|决策", DisplayName="当前AI行为")
	ELxAIActionType CurrentAction = ELxAIActionType::None;

	/** 最近一次完成分析得到的群体战场快照。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI|分析", DisplayName="当前AI战场快照")
	FLxAIBattleSnapshot CurrentBattleSnapshot;

	/** 因伤害行为被当前AI动态视为敌对的目标。 */
	TSet<TWeakObjectPtr<AActor>> DynamicHostileTargets;

	/** 每种行为最近一次结束的时间，用于执行配置冷却。 */
	TMap<ELxAIActionType, double> ActionEndTimes;

	/** 当前行为开始执行的世界时间。 */
	double CurrentActionStartTime = 0.0;

	/** 当前行为最近一次计算出的评分。 */
	float CurrentActionScore = 0.0f;

	/** 角色生成时保存的巡逻中心位置。 */
	FVector PatrolOrigin = FVector::ZeroVector;

	/** 定时执行自动分析与决策的计时器。 */
	FTimerHandle AutomaticDecisionTimer;
};
