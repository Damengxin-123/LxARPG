#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAITypes.generated.h"

class AActor;

/** AI对目标分析后得到的关系类型。 */
UENUM(BlueprintType, DisplayName="AI目标关系")
enum class ELxAITargetRelation : uint8
{
	Ignore UMETA(DisplayName="无关"),
	Assist UMETA(DisplayName="协助"),
	Hostile UMETA(DisplayName="敌对")
};

/** AI根据当前群体态势选择的战略类型。 */
UENUM(BlueprintType, DisplayName="AI战术战略")
enum class ELxAITacticalStrategy : uint8
{
	Idle UMETA(DisplayName="无威胁活动"),
	Engage UMETA(DisplayName="应对威胁"),
	Escape UMETA(DisplayName="脱离威胁")
};

/** 用少量分类概括单体或群体的主要移动意图。 */
UENUM(BlueprintType, DisplayName="AI移动意图")
enum class ELxAIMovementIntent : uint8
{
	Unknown UMETA(DisplayName="未知"),
	Advance UMETA(DisplayName="进攻"),
	Defend UMETA(DisplayName="防御"),
	Retreat UMETA(DisplayName="撤退")
};

/** 第一版内置执行器能够自动执行的AI行为。 */
UENUM(BlueprintType, DisplayName="AI行为类型")
enum class ELxAIActionType : uint8
{
	None UMETA(DisplayName="无"),
	Patrol UMETA(DisplayName="巡逻"),
	Alert UMETA(DisplayName="警戒"),
	Attack UMETA(DisplayName="攻击"),
	Defend UMETA(DisplayName="协助防御"),
	Heal UMETA(DisplayName="治疗友方"),
	Retreat UMETA(DisplayName="撤退")
};

/** 单个AI行为的限制、评分和群体占用配置。 */
USTRUCT(BlueprintType, DisplayName="AI行为规则")
struct LXARPG_API FLxAIActionRule
{
	GENERATED_BODY()

	/** 本规则对应的行为类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为", DisplayName="行为类型")
	ELxAIActionType ActionType = ELxAIActionType::None;

	/** 是否允许自动决策器选择该行为。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为", DisplayName="启用行为")
	bool bEnabled = true;

	/** 允许选择该行为的战略列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为", DisplayName="允许战略")
	TArray<ELxAITacticalStrategy> AllowedStrategies;

	/** 通过限制条件后的基础评分。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|评分", DisplayName="基础优先级")
	float BaseScore = 0.0f;

	/** 优势值对行为评分的影响权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|评分", DisplayName="优势值权重")
	float AdvantageWeight = 0.0f;

	/** 敌方进攻占比对行为评分的影响权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|评分", DisplayName="敌方进攻占比权重")
	float EnemyAdvanceRatioWeight = 0.0f;

	/** 协助方进攻占比对行为评分的影响权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|评分", DisplayName="协助方进攻占比权重")
	float AssistAdvanceRatioWeight = 0.0f;

	/** 最低状态友方的受伤程度对行为评分的影响权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|评分", DisplayName="友方受伤程度权重")
	float InjuredAllyWeight = 0.0f;

	/** 自身已损失生命比例对行为评分的影响权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|评分", DisplayName="自身受伤程度权重")
	float SelfInjuryWeight = 0.0f;

	/** 允许选择行为的最低优势值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|限制", DisplayName="最低优势值", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float MinAdvantage = -1.0f;

	/** 允许选择行为的最高优势值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|限制", DisplayName="最高优势值", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float MaxAdvantage = 1.0f;

	/** 允许选择行为的最低自身生命比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|限制", DisplayName="最低自身生命比例", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MinSelfHealthRatio = 0.0f;

	/** 允许选择行为的最高自身生命比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|限制", DisplayName="最高自身生命比例", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MaxSelfHealthRatio = 1.0f;

	/** 行为是否必须存在低状态协助目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|限制", DisplayName="需要受伤友方")
	bool bRequiresInjuredAlly = false;

	/** 同一群体允许同时执行该行为的最大人数，0表示不限制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|限制", DisplayName="群体最大执行人数", meta=(ClampMin="0"))
	int32 MaxGroupExecutors = 0;

	/** 行为被选中后至少保持的时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|切换", DisplayName="最短执行时间", meta=(ClampMin="0.0", Units="s"))
	float MinExecutionTime = 1.0f;

	/** 行为结束后再次选择它之前需要等待的时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|切换", DisplayName="行为冷却时间", meta=(ClampMin="0.0", Units="s"))
	float Cooldown = 0.0f;

	/** 判断本规则是否允许指定战略。 */
	bool AllowsStrategy(ELxAITacticalStrategy InStrategy) const
	{
		return AllowedStrategies.Contains(InStrategy);
	}
};

/** AI角色无需连接蓝图节点即可使用的控制参数。 */
USTRUCT(BlueprintType, DisplayName="AI控制配置")
struct LXARPG_API FLxAIControlConfig
{
	GENERATED_BODY()

	/** 是否启用自动感知、分析、决策和执行。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|基础", DisplayName="启用自动控制")
	bool bEnableAutomaticControl = true;

	/** AI视觉感知半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|感知", DisplayName="视觉感知半径", meta=(ClampMin="0.0", Units="cm"))
	float SightRadius = 2500.0f;

	/** 已发现目标离开该半径后允许视觉丢失。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|感知", DisplayName="视觉丢失半径", meta=(ClampMin="0.0", Units="cm"))
	float LoseSightRadius = 3000.0f;

	/** 感知共享记录超过该时间后失效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|感知", DisplayName="共享情报有效时间", meta=(ClampMin="0.1", Units="s"))
	float SharedPerceptionMaxAge = 3.0f;

	/** 自动分析和决策的时间间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="决策间隔", meta=(ClampMin="0.05", Units="s"))
	float DecisionInterval = 0.35f;

	/** 当前行为至少比旧行为高出该分数才允许提前切换。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="行为切换分差", meta=(ClampMin="0.0"))
	float ActionSwitchScoreMargin = 5.0f;

	/** 优势值低于该值时进入脱离威胁战略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="逃跑优势阈值", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float EscapeAdvantageThreshold = -0.45f;

	/** 自身生命比例低于该值时进入脱离威胁战略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="逃跑生命阈值", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EscapeHealthThreshold = 0.2f;

	/** 协助目标生命比例低于该值时视为需要治疗。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="友方低状态阈值", meta=(ClampMin="0.0", ClampMax="1.0"))
	float InjuredAllyThreshold = 0.45f;

	/** 速度低于该值时将目标动线概括为防御。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|分析", DisplayName="动线静止速度阈值", meta=(ClampMin="0.0", Units="cm/s"))
	float IntentStationarySpeed = 35.0f;

	/** 目标朝对方中心的径向速度超过该值时视为进攻或撤退。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|分析", DisplayName="动线径向速度阈值", meta=(ClampMin="0.0", Units="cm/s"))
	float IntentRadialSpeed = 50.0f;

	/** 接敌移动停止时与目标保持的距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="攻击接近距离", meta=(ClampMin="0.0", Units="cm"))
	float AttackAcceptanceRadius = 180.0f;

	/** 自动释放攻击技能允许的最大距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="攻击技能距离", meta=(ClampMin="0.0", Units="cm"))
	float AttackSkillRange = 220.0f;

	/** 自动攻击时优先释放的技能物品ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="攻击技能物品ID", meta=(Categories="物品.技能"))
	FGameplayTag AttackSkillItemId;

	/** 自动治疗时使用的技能物品ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="治疗技能物品ID", meta=(Categories="物品.技能"))
	FGameplayTag HealSkillItemId;

	/** 自动释放治疗技能允许的最大距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="治疗技能距离", meta=(ClampMin="0.0", Units="cm"))
	float HealSkillRange = 400.0f;

	/** 每次撤退行为尝试远离敌方中心的距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="撤退移动距离", meta=(ClampMin="0.0", Units="cm"))
	float RetreatDistance = 1200.0f;

	/** 无威胁巡逻时围绕出生点选择位置的半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|执行", DisplayName="巡逻半径", meta=(ClampMin="0.0", Units="cm"))
	float PatrolRadius = 800.0f;

	/** 角色总强度参与AI分析时使用的倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|分析", DisplayName="战力倍率", meta=(ClampMin="0.0"))
	float CombatStrengthMultiplier = 1.0f;

	/** 自动决策器依次过滤和评分的行为规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为", DisplayName="行为规则")
	TArray<FLxAIActionRule> ActionRules;
};

/** 某一方多个目标的移动意图概括结果。 */
USTRUCT(BlueprintType, DisplayName="AI群体动线摘要")
struct LXARPG_API FLxAIGroupIntentSummary
{
	GENERATED_BODY()

	/** 当前占比最高的移动意图。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|群体动线", DisplayName="主要动线")
	ELxAIMovementIntent DominantIntent = ELxAIMovementIntent::Unknown;

	/** 进攻目标的有效战力占比。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|群体动线", DisplayName="进攻占比")
	float AdvanceRatio = 0.0f;

	/** 防御目标的有效战力占比。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|群体动线", DisplayName="防御占比")
	float DefendRatio = 0.0f;

	/** 撤退目标的有效战力占比。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|群体动线", DisplayName="撤退占比")
	float RetreatRatio = 0.0f;

	/** 第一高占比与第二高占比之间的差值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|群体动线", DisplayName="主导程度")
	float Dominance = 0.0f;

	/** 当前有效样本对群体动线结论的可信程度。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|群体动线", DisplayName="可信度")
	float Confidence = 0.0f;
};

/** 群体共享感知结果经过压缩后生成的固定大小战场快照。 */
USTRUCT(BlueprintType, DisplayName="AI战场快照")
struct LXARPG_API FLxAIBattleSnapshot
{
	GENERATED_BODY()

	/** 当前参与分析的敌对目标数量。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌对目标数量")
	int32 EnemyCount = 0;

	/** 当前参与分析的协助目标数量，包含自己。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="协助目标数量")
	int32 AssistCount = 0;

	/** 当前敌对方有效战力。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌对方有效战力")
	float EnemyPower = 0.0f;

	/** 当前协助方有效战力。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="协助方有效战力")
	float AssistPower = 0.0f;

	/** 归一化后的协助方战力优势，范围为-1到1。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="优势值")
	float AdvantageScore = 0.0f;

	/** 敌对方群体移动意图摘要。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌对方动线摘要")
	FLxAIGroupIntentSummary EnemyIntent;

	/** 协助方群体移动意图摘要。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="协助方动线摘要")
	FLxAIGroupIntentSummary AssistIntent;

	/** 协助方所有有效目标的平均位置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="协助方中心位置")
	FVector AssistCenter = FVector::ZeroVector;

	/** 敌对方所有有效目标的平均位置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌对方中心位置")
	FVector EnemyCenter = FVector::ZeroVector;

	/** 当前威胁值最高的敌对目标。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最高威胁目标")
	TObjectPtr<AActor> HighestThreatEnemy = nullptr;

	/** 当前生命比例最低且低于配置阈值的协助目标。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最低状态友方")
	TObjectPtr<AActor> LowestStateAlly = nullptr;

	/** 最低状态友方当前的生命比例。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最低友方生命比例")
	float LowestAllyHealthRatio = 1.0f;

	/** 当前共享情报中是否存在有效威胁。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="存在有效威胁")
	bool bHasThreat = false;
};
