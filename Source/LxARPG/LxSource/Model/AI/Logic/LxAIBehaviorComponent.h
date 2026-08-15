#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxAIBehaviorComponent.generated.h"

class ALxAICharacter;
class AAIController;
class ULxCharacterMoveComponent;

/** 将AI决策结果转换为角色通用移动、近战和技能组件调用。 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="AI行为组件")
class LXARPG_API ULxAIBehaviorComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建不使用独立Tick的AI行为组件。 */
	ULxAIBehaviorComponent();

	/** 缓存所属AI角色、出生位置和角色通用移动组件。 */
	virtual void BaseComponentInitialize() override;

	/** 由指定行为自己的条件判断当前是否可以执行，不读取通用行为限制参数。 */
	UFUNCTION(BlueprintPure, Category="AI|行为", DisplayName="AI行为是否可执行")
	bool CanExecuteBehavior(ELxAIActionType InActionType, const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 使用角色已有组件尝试执行指定AI行为，并将执行结果返回给控制器继续匹配。 */
	UFUNCTION(BlueprintCallable, Category="AI|行为", DisplayName="执行AI行为")
	ELxAIBehaviorExecutionResult ExecuteBehavior(ELxAIActionType InActionType,
		const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 停止当前移动并清除AI控制器的关注目标。 */
	UFUNCTION(BlueprintCallable, Category="AI|行为", DisplayName="停止AI行为")
	void StopBehavior();

	/** 获取当前是否仍处于尚未达到配置距离的逃跑过程。 */
	UFUNCTION(BlueprintPure, Category="AI|行为|逃跑", DisplayName="逃跑行为是否进行中")
	bool IsRetreatInProgress() const { return bRetreatInProgress; }

	/** 根据本轮敌对目标与决策结果更新追近状态，并在达到配置距离时结束逃跑。 */
	void UpdateRetreatProgress(const FLxAIBattleSnapshot& InBattleSnapshot, bool bInShouldRetreat);

private:
	/** 判断巡逻行为是否满足自身的无威胁条件。 */
	bool CanExecutePatrol(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 判断警戒行为是否可以作为当前安全回退行为。 */
	bool CanExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 判断攻击行为是否具有有效敌方目标。 */
	bool CanExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 判断防守行为是否存在需要应对的敌方目标。 */
	bool CanExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 判断治疗行为是否具有有效友方目标、技能配置和技能组件。 */
	bool CanExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 判断逃跑行为是否已经开始，或当前存在可以用于开始逃跑的最近敌方目标。 */
	bool CanExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 使用通用移动组件执行无威胁巡逻。 */
	ELxAIBehaviorExecutionResult ExecutePatrol();

	/** 停止移动并关注当前最高威胁目标。 */
	ELxAIBehaviorExecutionResult ExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动和技能组件接近并攻击最高威胁目标。 */
	ELxAIBehaviorExecutionResult ExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动组件靠近协助方中心并面向威胁。 */
	ELxAIBehaviorExecutionResult ExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动和技能组件接近并治疗最低状态友方。 */
	ELxAIBehaviorExecutionResult ExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 根据最近敌方的实时位置，使用通用移动组件前往反方向的邻近可达点。 */
	ELxAIBehaviorExecutionResult ExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 获取所属AI角色的AI控制器。 */
	AAIController* GetOwnerAIController() const;

	/** 清除本次逃跑的起点、方向和敌方追近检测状态。 */
	void ResetRetreatState();

	/** 当前组件所属的AI角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxAICharacter> OwnerAICharacter = nullptr;

	/** 玩家输入与AI行为共同调用的角色移动组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterMoveComponent> CharacterMoveComponent = nullptr;

	/** AI角色开始运行时记录的巡逻中心位置。 */
	FVector PatrolOrigin = FVector::ZeroVector;

	/** 本次逃跑用于累计实际移动距离的开始位置。 */
	FVector RetreatStartLocation = FVector::ZeroVector;

	/** 最近一次有效的逃离方向，用于敌方暂时丢失后继续分段逃跑。 */
	FVector LastRetreatDirection = FVector::ZeroVector;

	/** 上一轮追近检测使用的敌方角色。 */
	TWeakObjectPtr<AActor> LastRetreatEnemy;

	/** 上一轮追近检测时角色与敌方之间的水平距离，单位为厘米。 */
	float LastRetreatEnemyDistance = 0.0f;

	/** 当前是否仍需累计逃跑距离。 */
	bool bRetreatInProgress = false;

	/** 上一轮是否存在满足逃跑决策的有效敌方目标。 */
	bool bHadRetreatEnemy = false;

	/** 上一轮敌方是否正在缩短与角色之间的距离。 */
	bool bRetreatEnemyWasClosing = false;

	/** 达到逃跑距离后是否等待威胁消失、切换或再次追近，避免每轮决策立即重启逃跑。 */
	bool bRetreatCompletionBlocked = false;

	/** 完成本次逃跑时仍在检测范围内的敌方角色。 */
	TWeakObjectPtr<AActor> CompletedRetreatEnemy;

	/** 完成本次逃跑时与敌方之间的水平距离，单位为厘米。 */
	float CompletedRetreatEnemyDistance = 0.0f;
};
