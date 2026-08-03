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

	/** 使用角色已有组件执行指定AI行为，不在本组件内重复实现角色能力。 */
	UFUNCTION(BlueprintCallable, Category="AI|行为", DisplayName="执行AI行为")
	void ExecuteBehavior(ELxAIActionType InActionType, const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 停止当前移动并清除AI控制器的关注目标。 */
	UFUNCTION(BlueprintCallable, Category="AI|行为", DisplayName="停止AI行为")
	void StopBehavior();

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

	/** 判断逃跑行为是否存在有效威胁。 */
	bool CanExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot) const;

	/** 使用通用移动组件执行无威胁巡逻。 */
	void ExecutePatrol();

	/** 停止移动并关注当前最高威胁目标。 */
	void ExecuteAlert(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动和技能组件接近并攻击最高威胁目标。 */
	void ExecuteAttack(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动组件靠近协助方中心并面向威胁。 */
	void ExecuteDefend(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动和技能组件接近并治疗最低状态友方。 */
	void ExecuteHeal(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 使用通用移动组件前往远离敌方中心的可达位置。 */
	void ExecuteRetreat(const FLxAIBattleSnapshot& InBattleSnapshot);

	/** 获取所属AI角色的AI控制器。 */
	AAIController* GetOwnerAIController() const;

	/** 当前组件所属的AI角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxAICharacter> OwnerAICharacter = nullptr;

	/** 玩家输入与AI行为共同调用的角色移动组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterMoveComponent> CharacterMoveComponent = nullptr;

	/** AI角色开始运行时记录的巡逻中心位置。 */
	FVector PatrolOrigin = FVector::ZeroVector;
};
