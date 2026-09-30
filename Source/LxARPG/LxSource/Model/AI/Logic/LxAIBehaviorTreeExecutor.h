#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAIBehaviorTreeAsset.h"
#include "LxARPG/LxSource/Model/AI/LxAIControlAnalysis.h"
#include "LxAIBehaviorTreeExecutor.generated.h"

class AActor;
class ALxAICharacter;
class ALxAIRouteActor;
class ULxAIBehaviorTreeAsset;
class ULxAIBehaviorTreeNodeData;

/** 行为树叶节点变化时使用的原生回调。 */
DECLARE_DELEGATE_OneParam(FLxAIBehaviorTreeLeafChanged, ELxAIBehaviorAction);

/**
 * 每个 AI 控制器独立持有的行为树执行器。
 * 按阶段内排列序号执行叶节点，并通过角色通用行为控制与技能模块产生实际行为。
 */
UCLASS()
class LXARPG_API ULxAIBehaviorTreeExecutor : public UObject
{
	GENERATED_BODY()
public:
	/** 绑定本次执行会话所属角色。 */
	void Initialize(ALxAICharacter* InCharacter);

	/** 推进当前分析分支；返回一次性受击分支是否已经完整执行。 */
	bool TickExecution(const ULxAIBehaviorTreeAsset* InAsset, const FLxAIAnalysisDecision& InDecision,
		AActor* InKnownEnemy, const FVector& InLastKnownEnemyLocation, float InDeltaSeconds);

	/** 停止移动并清空当前阶段和叶节点进度。 */
	void ResetExecution();

	/** 获取当前正在运行的叶行为。 */
	ELxAIBehaviorAction GetCurrentAction() const { return CurrentAction; }

	/** 获取当前正在运行的叶节点标识。 */
	FGuid GetCurrentActionNodeId() const { return CurrentActionNodeId; }

	/** 是否允许分析事件抢占当前行为；尚未开始、已经完成或失败的行为不持有执行锁。 */
	bool CanInterruptCurrentAction() const;
	/** 本轮是否实际抵达定点巡逻目的地或路线巡逻路径点，供追击返回保护使用。 */
	bool HasReachedPatrolDestinationThisTick() const { return bReachedPatrolDestinationThisTick; }

	/** 当前叶节点变化通知。 */
	FLxAIBehaviorTreeLeafChanged OnLeafChanged;

private:
	enum class ELeafResult : uint8 { Running, Completed, Failed };

	/** 进入分析选中的阶段并按排列序号缓存其行为。 */
	bool EnterPhase(const ULxAIBehaviorTreeAsset* InAsset, const FLxAIAnalysisDecision& InDecision);
	/** 执行一个叶节点。 */
	ELeafResult TickLeaf(const ULxAIBehaviorTreeNodeData& InNode, AActor* InEnemy,
		const FVector& InEnemyLocation, float InDeltaSeconds);
	/** 切换当前叶节点并广播。 */
	void ChangeLeaf(const ULxAIBehaviorTreeNodeData* InNode);
	/** 结束当前叶的瞬时运行状态。 */
	void ResetLeafProgress(bool bStopMovement);
	/** 请求移动到指定位置。 */
	bool MoveToLocation(const FVector& InLocation, float InAcceptanceRadius);
	/** 在点位范围内挑选一个距离角色足够远且可到达的巡逻目标。 */
	bool ChoosePointPatrolDestination(const class ALxAIPointActor& InPoint, const FVector& InSelfLocation);
	/** 为当前固定路线逃跑路径点选取一次偏离范围内的可导航目标。 */
	void ChooseRouteFleeDestination(const FVector& InRoutePoint, float InDeviationMeters);
	/** 检查随机逃跑目标与角色之间的导航直线和角色宽度碰撞通道。 */
	bool IsRandomFleeSegmentClear(const FVector& InDestination) const;
	/** 优先远离敌人，逐个尝试不同方向及距离的无遮挡导航点。 */
	bool ChooseRandomFleeDestination(const ULxAIBehaviorTreeNodeData& InNode, const FVector& InEnemyLocation);
	/** 获取场景导航注册表。 */
	class ULxAINavigationRegistry* GetNavigationRegistry() const;
	/** 释放节点指定的技能。 */
	ELeafResult ReleaseSkill(const ULxAIBehaviorTreeNodeData& InNode, AActor* InTarget,
		const FVector& InTargetLocation);

	UPROPERTY(Transient)
	TObjectPtr<ALxAICharacter> Character;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxAIBehaviorTreeNodeData>> OrderedActions;
	FGuid CurrentPhaseId;
	int32 CurrentActionIndex = INDEX_NONE;
	ELxAIBehaviorAction CurrentAction = ELxAIBehaviorAction::Wait;
	FGuid CurrentActionNodeId;
	double LeafStartTime = 0.0;
	double WaitStartTime = -1.0;
	TWeakObjectPtr<ALxAIRouteActor> ActiveRoute;
	/** 当前路线在进入叶节点时复制的世界路径点，允许原场景 Actor 临时卸载。 */
	TArray<FVector> CachedRoutePoints;
	/** 本轮定点巡逻选中的导航目标。 */
	FVector PointPatrolDestination = FVector::ZeroVector;
	/** 是否已为当前巡逻叶节点选中导航目标。 */
	bool bHasPointPatrolDestination = false;
	int32 RoutePointIndex = 0;
	int32 RouteDirection = 1;
	/** 当前固定路线逃跑路径点选中的导航目标，抵达前保持不变。 */
	FVector RouteFleeDestination = FVector::ZeroVector;
	/** 当前路径点是否已生成固定路线逃跑目标。 */
	bool bHasRouteFleeDestination = false;
	/** 本次随机逃跑正在前往的目标；失败后用于避免立刻重复选择。 */
	FVector RandomFleeDestination = FVector::ZeroVector;
	/** 随机逃跑是否持有尚未抵达的目标。 */
	bool bHasRandomFleeDestination = false;
	/** 最近一次确认有移动进展的位置。 */
	FVector RandomFleeProgressLocation = FVector::ZeroVector;
	/** 随机逃跑连续未产生移动进展的秒数。 */
	float RandomFleeStalledSeconds = 0.0f;
	/** 固定路线逃跑已到终点；同一阶段不会因低血量持续重新从起点执行。 */
	bool bRouteFleeCompleted = false;
	/** 当前叶节点的导航问题是否已记录，避免加载阶段重复输出告警。 */
	bool bLoggedNavigationFailure = false;
	/** 死亡响应已执行完毕，等待角色生命周期销毁。 */
	bool bDeathActionCompleted = false;
	/** 当前叶节点最近一次执行是否仍在运行，防止提前锁住仅被选中但尚未执行的后续节点。 */
	bool bCurrentLeafRunning = false;
	/** 当前行为树叶节点是否持有面向敌人的请求，仅释放自己持有的引用。 */
	bool bHasBehaviorFacingRequest = false;
	/** 每轮执行开始清零；抵达巡逻目的地后置为真，即使同轮完成叶节点也保留。 */
	bool bReachedPatrolDestinationThisTick = false;
};
