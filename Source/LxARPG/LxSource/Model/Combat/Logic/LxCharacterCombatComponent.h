#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/CloseCombat/Logic/LxCharacterCloseCombatComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxCharacterCombatComponent.generated.h"

/** 角色战斗组件，统一管理技能释放和近身战斗 UObject 模块。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色战斗组件")
class LXARPG_API ULxCharacterCombatComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 创建唯一角色战斗组件及默认战斗模块。 */
	ULxCharacterCombatComponent();

	/** 初始化技能释放和近身战斗模块。 */
	virtual void BaseComponentInitialize() override;

	/** 关闭全部战斗模块。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取技能释放模块。 */
	UFUNCTION(BlueprintPure, Category="角色|战斗", DisplayName="获取技能释放模块")
	ULxSkillCastModule* GetSkillCastModule() const { return SkillCastModule; }

	/** 获取近身战斗模块。 */
	UFUNCTION(BlueprintPure, Category="角色|战斗", DisplayName="获取近身战斗模块")
	ULxCharacterCloseCombatModule* GetCloseCombatModule() const { return CloseCombatModule; }

	/** 判断当前是否允许开始技能释放。 */
	bool CanStartSkillCast() const;

	/** 判断当前是否允许开始近身战斗行为。 */
	bool CanStartCloseCombat() const;

	/** 接收模块数据变化并广播统一组件事件。 */
	void NotifyCombatModuleDataChanged();

	/** 请求广播技能动作动画。 */
	void RequestPlaySkillActionAnimation(float InSkillReleaseDuration);

	/** 请求广播技能动作动画结束。 */
	void RequestStopSkillActionAnimation();

	/** 近战攻击命中目标事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|战斗|近身战斗|攻击", DisplayName="近战攻击命中目标事件")
	FOnLxMeleeAttackHit OnMeleeAttackHit;

	/** 近战攻击执行结束事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|战斗|近身战斗|攻击", DisplayName="近战攻击执行结束事件")
	FOnLxMeleeAttackEnded OnMeleeAttackEnded;

	/** 格挡时被击中事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|战斗|近身战斗|格挡", DisplayName="格挡时被击中事件")
	FOnLxBlockHit OnBlockHit;

	/** 格挡执行结束事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|战斗|近身战斗|格挡", DisplayName="格挡执行结束事件")
	FOnLxBlockEnded OnBlockEnded;

private:
	/** 服务端校验并执行技能释放输入。 */
	UFUNCTION(Server, Reliable, Category="角色|战斗|网络", DisplayName="服务端处理技能释放输入")
	void ServerHandleSkillItemReleaseInput(FGameplayTag InSkillItemIDTag, ELxSkillReleaseInputState InInputState,
		AActor* InTargetActor, FVector_NetQuantize InAimLocation, bool bInHasAimLocation,
		FVector_NetQuantizeNormal InAimDirection, bool bInHasAimDirection);

	/** 广播技能动作动画。 */
	UFUNCTION(NetMulticast, Reliable, Category="角色|战斗|网络", DisplayName="广播技能动作动画")
	void MulticastPlaySkillActionAnimation(float InSkillReleaseDuration);

	/** 广播技能动作动画结束。 */
	UFUNCTION(NetMulticast, Reliable, Category="角色|战斗|网络", DisplayName="广播技能动作动画结束")
	void MulticastStopSkillActionAnimation();

	/** 转发近战攻击命中事件。 */
	UFUNCTION(Category="角色|战斗|近身战斗", DisplayName="转发近战攻击命中")
	void HandleMeleeAttackHit(const FLxMeleeAttackHitResult& HitResult);

	/** 转发近战攻击结束事件。 */
	UFUNCTION(Category="角色|战斗|近身战斗", DisplayName="转发近战攻击结束")
	void HandleMeleeAttackEnded(const FLxMeleeAttackEndContext& EndContext);

	/** 转发格挡命中事件。 */
	UFUNCTION(Category="角色|战斗|近身战斗", DisplayName="转发格挡命中")
	void HandleBlockHit(const FLxBlockHitResult& BlockResult);

	/** 转发格挡结束事件。 */
	UFUNCTION(Category="角色|战斗|近身战斗", DisplayName="转发格挡结束")
	void HandleBlockEnded(const FLxBlockEndContext& EndContext);

	/** 注册需要由该组件复制的战斗 UObject 模块。 */
	void RegisterReplicatedModules();

	/** 技能释放模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|战斗", DisplayName="技能释放模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxSkillCastModule> SkillCastModule;

	/** 近身战斗模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|战斗", DisplayName="近身战斗模块", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxCharacterCloseCombatModule> CloseCombatModule;

	/** 角色战斗组件是否已经初始化。 */
	bool bCombatInitialized = false;

	/** 允许技能释放模块调用统一组件的网络入口。 */
	friend class ULxSkillCastModule;
};
