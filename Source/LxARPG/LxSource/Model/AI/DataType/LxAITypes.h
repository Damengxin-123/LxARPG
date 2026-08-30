#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAITypes.generated.h"

class AActor;

/** AI根据稳定阵营规则和明确受击事件得到的目标关系。 */
UENUM(BlueprintType, DisplayName="AI目标关系")
enum class ELxAITargetRelation : uint8
{
	Ignore UMETA(DisplayName="无关"),
	Assist UMETA(DisplayName="友方"),
	Hostile UMETA(DisplayName="敌方")
};

/** 当前AI根据敌友数量、有效强度和状态对比得到的简化局势等级。 */
UENUM(BlueprintType, DisplayName="AI局势等级")
enum class ELxAISituationLevel : uint8
{
	NoThreat UMETA(DisplayName="无敌对目标"),
	Advantage UMETA(DisplayName="明显占优"),
	Balanced UMETA(DisplayName="接近均势"),
	Disadvantage UMETA(DisplayName="明显劣势"),
	SelfDanger UMETA(DisplayName="自身危险")
};

/** 当前AI私有目标缓存记录感知信息时使用的来源类型。 */
UENUM(BlueprintType, DisplayName="AI感知来源")
enum class ELxAIPerceptionSource : uint8
{
	Unknown UMETA(DisplayName="未知"),
	Sight UMETA(DisplayName="视觉感知"),
	Range UMETA(DisplayName="范围感知"),
	Damage UMETA(DisplayName="受击感知"),
	Interaction UMETA(DisplayName="交互感知"),
	Effect UMETA(DisplayName="效果感知")
};

/** 第一版AI行为组件能够独立检查并执行的行为。 */
UENUM(BlueprintType, DisplayName="AI行为类型")
enum class ELxAIActionType : uint8
{
	None UMETA(DisplayName="无"),
	Patrol UMETA(DisplayName="巡逻"),
	Alert UMETA(DisplayName="警戒"),
	Attack UMETA(DisplayName="攻击"),
	Defend UMETA(DisplayName="防守"),
	Heal UMETA(DisplayName="治疗友方"),
	Retreat UMETA(DisplayName="逃跑")
};

/** AI行为组件尝试执行行为后返回给控制器的结果。 */
UENUM(BlueprintType, DisplayName="AI行为执行结果")
enum class ELxAIBehaviorExecutionResult : uint8
{
	Failed UMETA(DisplayName="执行失败"),
	Started UMETA(DisplayName="已开始"),
	InProgress UMETA(DisplayName="执行中"),
	Waiting UMETA(DisplayName="等待条件")
};

/** 单个局势等级按顺序匹配的行为候选，不包含任何行为限制参数。 */
USTRUCT(BlueprintType, DisplayName="AI局势行为集合")
struct LXARPG_API FLxAISituationBehaviorSet
{
	GENERATED_BODY()

	/** 本候选集合对应的局势等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="局势等级")
	ELxAISituationLevel Situation = ELxAISituationLevel::NoThreat;

	/** 按顺序交给行为组件检查的行为候选。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="行为候选")
	TArray<ELxAIActionType> BehaviorCandidates;
};

/** 每个AI角色独立感知、数值比较和行为匹配使用的控制配置。 */
USTRUCT(BlueprintType, DisplayName="AI控制配置")
struct LXARPG_API FLxAIControlConfig
{
	GENERATED_BODY()

	/** 是否启用当前角色独立的自动感知、决策和执行。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|基础", DisplayName="启用自动控制")
	bool bEnableAutomaticControl = true;

	/** AI视觉感知半径，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|感知", DisplayName="视觉感知半径", meta=(ClampMin="0.0", Units="m"))
	float SightRadius = 25.0f;

	/** 已发现目标离开该半径后允许视觉丢失，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|感知", DisplayName="视觉丢失半径", meta=(ClampMin="0.0", Units="m"))
	float LoseSightRadius = 30.0f;

	/** 当前AI私有目标记录超过该时间后失效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|感知", DisplayName="目标记忆有效时间", meta=(ClampMin="0.1", Units="s"))
	float TargetMemoryMaxAge = 3.0f;

	/** 自动刷新目标数值并重新匹配行为的时间间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="决策间隔", meta=(ClampMin="0.05", Units="s"))
	float DecisionInterval = 0.35f;

	/** 非紧急行为至少持续该时间后才允许被另一个仍可执行的普通行为替换。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="行为最短持续时间", meta=(ClampMin="0.0", Units="s"))
	float MinimumActionDuration = 0.8f;

	/** 当前局势跨过阈值后需要额外回退的分数范围，用于避免在阈值附近频繁切换。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|阈值", DisplayName="局势切换滞后", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SituationHysteresis = 0.08f;

	/** 数量差异参与综合优势值计算的权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|对比", DisplayName="数量对比权重", meta=(ClampMin="0.0"))
	float NumberComparisonWeight = 0.2f;

	/** 有效强度差异参与综合优势值计算的权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|对比", DisplayName="强度对比权重", meta=(ClampMin="0.0"))
	float StrengthComparisonWeight = 0.6f;

	/** 双方平均状态差异参与综合优势值计算的权重。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|对比", DisplayName="状态对比权重", meta=(ClampMin="0.0"))
	float StateComparisonWeight = 0.2f;

	/** 综合优势值达到该值时进入明显占优局势。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|阈值", DisplayName="明显占优阈值", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float AdvantageThreshold = 0.25f;

	/** 综合优势值低于该值时进入明显劣势局势。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|阈值", DisplayName="明显劣势阈值", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float DisadvantageThreshold = -0.25f;

	/** 自身生命比例低于该值时优先进入自身危险局势。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|阈值", DisplayName="自身危险状态阈值", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SelfDangerStateThreshold = 0.2f;

	/** 友方生命比例低于该值时将其记为最低状态友方。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|阈值", DisplayName="友方低状态阈值", meta=(ClampMin="0.0", ClampMax="1.0"))
	float InjuredAllyThreshold = 0.45f;

	/** 不同局势等级按顺序尝试的行为候选。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="局势行为集合")
	TArray<FLxAISituationBehaviorSet> SituationBehaviorSets;

	/** 当前局势没有任何候选行为通过自身检查时使用的安全回退行为。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策", DisplayName="安全回退行为")
	ELxAIActionType FallbackAction = ELxAIActionType::Alert;

	/** 接敌移动停止时与目标保持的距离，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|攻击", DisplayName="攻击接近距离", meta=(ClampMin="0.0", Units="m"))
	float AttackAcceptanceRadius = 1.8f;

	/** 自动释放攻击技能允许的最大距离，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|攻击", DisplayName="攻击技能距离", meta=(ClampMin="0.0", Units="m"))
	float AttackSkillRange = 2.2f;

	/** 攻击行为使用的技能物品ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|攻击", DisplayName="攻击技能物品ID", meta=(Categories="物品.技能"))
	FGameplayTag AttackSkillItemId;

	/** 治疗行为使用的技能物品ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|治疗", DisplayName="治疗技能物品ID", meta=(Categories="物品.技能"))
	FGameplayTag HealSkillItemId;

	/** 治疗行为允许释放技能的最大距离，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|治疗", DisplayName="治疗技能距离", meta=(ClampMin="0.0", Units="m"))
	float HealSkillRange = 4.0f;

	/** 巡逻行为围绕角色出生点选择位置的半径，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|巡逻", DisplayName="巡逻半径", meta=(ClampMin="0.0", Units="m"))
	float PatrolRadius = 8.0f;

	/** 单次逃跑从开始位置至少需要移动的距离，达到后才允许结束本次逃跑，配置单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|行为|逃跑", DisplayName="逃跑距离", meta=(ClampMin="0.0", Units="m"))
	float RetreatDistance = 6.0f;

	/** 当前AI基础强度参与数值对比时使用的倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|决策|对比", DisplayName="战力倍率", meta=(ClampMin="0.0"))
	float CombatStrengthMultiplier = 1.0f;
};

/** 当前AI仅根据自身感知目标生成的数值化战场快照。 */
USTRUCT(BlueprintType, DisplayName="AI战场快照")
struct LXARPG_API FLxAIBattleSnapshot
{
	GENERATED_BODY()

	/** 当前AI私有缓存中有效的敌方目标数量。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌方目标数量")
	int32 EnemyCount = 0;

	/** 当前AI及其直接感知到的友方数量。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="友方目标数量")
	int32 AssistCount = 0;

	/** 敌方目标未乘状态系数前的基础强度总和。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌方基础强度")
	float EnemyBaseStrength = 0.0f;

	/** 友方目标未乘状态系数前的基础强度总和。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="友方基础强度")
	float AssistBaseStrength = 0.0f;

	/** 敌方基础强度乘当前状态后的有效强度总和。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌方有效强度")
	float EnemyEffectiveStrength = 0.0f;

	/** 友方基础强度乘当前状态后的有效强度总和。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="友方有效强度")
	float AssistEffectiveStrength = 0.0f;

	/** 敌方目标当前归一化状态的平均值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌方平均状态")
	float EnemyAverageState = 0.0f;

	/** 友方目标当前归一化状态的平均值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="友方平均状态")
	float AssistAverageState = 0.0f;

	/** 当前AI自身的归一化状态值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="自身状态")
	float SelfState = 1.0f;

	/** 友方数量除以敌方数量后的直接对比值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="数量优势比")
	float NumberAdvantageRatio = 1.0f;

	/** 友方有效强度除以敌方有效强度后的直接对比值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="强度优势比")
	float StrengthAdvantageRatio = 1.0f;

	/** 友方平均状态除以敌方平均状态后的直接对比值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="状态优势比")
	float StateAdvantageRatio = 1.0f;

	/** 数量、有效强度和状态加权后的综合优势值，范围为-1到1。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="综合优势值")
	float AdvantageScore = 0.0f;

	/** 当前直接感知到的友方平均位置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="友方中心位置")
	FVector AssistCenter = FVector::ZeroVector;

	/** 当前直接感知到的敌方平均位置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="敌方中心位置")
	FVector EnemyCenter = FVector::ZeroVector;

	/** 当前有效强度与距离组合后威胁最高的敌方目标。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最高威胁目标")
	TObjectPtr<AActor> HighestThreatEnemy = nullptr;

	/** 当前AI与全部有效敌方比较后距离最近的敌方目标。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最近敌方目标")
	TObjectPtr<AActor> NearestEnemy = nullptr;

	/** 当前状态最低且低于角色配置阈值的友方目标。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最低状态友方")
	TObjectPtr<AActor> LowestStateAlly = nullptr;

	/** 最低状态友方当前的归一化状态值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="最低友方状态")
	float LowestAllyState = 1.0f;

	/** 当前AI私有目标缓存中是否存在有效敌方。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|战场快照", DisplayName="存在有效威胁")
	bool bHasThreat = false;
};
