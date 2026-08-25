#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatModuleBase.h"
#include "LxARPG/LxSource/Model/CloseCombat/DataType/LxCloseCombatTypes.h"
#include "LxCharacterCloseCombatComponent.generated.h"

class ALxBaseCharacter;
class UPrimitiveComponent;

/** 近战攻击命中有效目标时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxMeleeAttackHit, const FLxMeleeAttackHitResult&, HitResult);

/** 近战攻击行为结束时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxMeleeAttackEnded, const FLxMeleeAttackEndContext&, EndContext);

/** 格挡期间盾牌被武器击中并完成格挡判定时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxBlockHit, const FLxBlockHitResult&, BlockResult);

/** 格挡行为结束时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxBlockEnded, const FLxBlockEndContext&, EndContext);

/** 角色近身战斗模块，负责攻击与格挡的互斥状态、碰撞判定和结果广播。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色近身战斗模块")
class LXARPG_API ULxCharacterCloseCombatModule : public ULxCharacterCombatModuleBase
{
	GENERATED_BODY()

public:
	/** 创建角色近身战斗模块。 */
	ULxCharacterCloseCombatModule();

	/** 绑定统一战斗组件并缓存所属角色。 */
	virtual void InitializeModule(ULxCharacterCombatComponent* InOwnerComponent) override;

	/** 解绑碰撞事件并清理当前行为。 */
	virtual void ShutdownModule() override;

	/** 在空闲状态下开始一次近战攻击；请求无效或组件正忙时返回 false。 */
	UFUNCTION(BlueprintCallable, Category="角色|近身战斗|攻击", DisplayName="开始近战攻击")
	bool StartAttack(const FLxMeleeAttackRequest& InAttackRequest);

	/** 结束指定来源发起的当前近战攻击；来源不匹配时不会结束。 */
	UFUNCTION(BlueprintCallable, Category="角色|近身战斗|攻击", DisplayName="结束近战攻击")
	bool EndAttack(ULxSkillUnitGroup* InSourceSkillUnitGroup = nullptr);

	/** 在空闲状态下开始格挡；请求无效或组件正忙时返回 false。 */
	UFUNCTION(BlueprintCallable, Category="角色|近身战斗|格挡", DisplayName="开始格挡")
	bool StartBlock(const FLxBlockRequest& InBlockRequest);

	/** 结束当前格挡行为。 */
	UFUNCTION(BlueprintCallable, Category="角色|近身战斗|格挡", DisplayName="结束格挡")
	bool EndBlock();

	/** 获取当前近身战斗状态。 */
	UFUNCTION(BlueprintPure, Category="角色|近身战斗|状态", DisplayName="获取近身战斗状态")
	ELxCloseCombatState GetCloseCombatState() const { return CloseCombatState; }

	/** 判断当前是否可以开始新的攻击或格挡行为。 */
	UFUNCTION(BlueprintPure, Category="角色|近身战斗|状态", DisplayName="近身战斗是否空闲")
	bool IsCloseCombatIdle() const { return CloseCombatState == ELxCloseCombatState::Idle; }

	/** 获取当前攻击请求副本。 */
	UFUNCTION(BlueprintPure, Category="角色|近身战斗|攻击", DisplayName="获取当前攻击请求")
	FLxMeleeAttackRequest GetCurrentAttackRequest() const { return CurrentAttackRequest; }

	/** 获取当前格挡请求副本。 */
	UFUNCTION(BlueprintPure, Category="角色|近身战斗|格挡", DisplayName="获取当前格挡请求")
	FLxBlockRequest GetCurrentBlockRequest() const { return CurrentBlockRequest; }

	/** 近战攻击命中有效目标事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|近身战斗|攻击|事件", DisplayName="近战攻击命中目标事件")
	FOnLxMeleeAttackHit OnMeleeAttackHit;

	/** 近战攻击执行结束事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|近身战斗|攻击|事件", DisplayName="近战攻击执行结束事件")
	FOnLxMeleeAttackEnded OnMeleeAttackEnded;

	/** 格挡期间盾牌被武器击中事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|近身战斗|格挡|事件", DisplayName="格挡时被击中事件")
	FOnLxBlockHit OnBlockHit;

	/** 格挡行为结束事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|近身战斗|格挡|事件", DisplayName="格挡执行结束事件")
	FOnLxBlockEnded OnBlockEnded;

	/** 当前默认始终成功，后续可在蓝图或 C++ 中接入体力消耗与破防计算。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="角色|近身战斗|格挡", DisplayName="判定格挡是否成功")
	bool EvaluateBlockHit(const FLxBlockHitResult& InBlockHit) const;
	virtual bool EvaluateBlockHit_Implementation(const FLxBlockHitResult& InBlockHit) const;

private:
	/** 接收当前武器碰撞体产生的开始重叠事件。 */
	UFUNCTION()
	void HandleWeaponBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 接收当前盾牌碰撞体产生的开始重叠事件。 */
	UFUNCTION()
	void HandleShieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 验证结束请求是否来自当前攻击行为。 */
	bool MatchesCurrentAttackSource(const ULxSkillUnitGroup* InSourceSkillUnitGroup) const;

	/** 将当前攻击作为被格挡结果中断。 */
	void InterruptCurrentAttackByBlock();

	/** 解绑武器碰撞事件并广播攻击结束。 */
	void FinishAttack();

	/** 解绑盾牌碰撞事件并广播格挡结束。 */
	void FinishBlock();

	/** 缓存的组件所属角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxBaseCharacter> OwnerCharacter = nullptr;

	/** 当前互斥的近身战斗行为状态。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|近身战斗|状态", DisplayName="当前近身战斗状态", meta=(AllowPrivateAccess="true"))
	ELxCloseCombatState CloseCombatState = ELxCloseCombatState::Idle;

	/** 当前已经被组件接受并执行的攻击请求。 */
	UPROPERTY(Transient)
	FLxMeleeAttackRequest CurrentAttackRequest;

	/** 当前已经被组件接受并执行的格挡请求。 */
	UPROPERTY(Transient)
	FLxBlockRequest CurrentBlockRequest;

	/** 当前攻击已经对外发布的有效命中次数。 */
	int32 CurrentAttackHitCount = 0;

	/** 当前攻击已经命中过的目标，用于限制同一目标重复命中。 */
	TSet<TWeakObjectPtr<AActor>> CurrentHitTargets;
};
