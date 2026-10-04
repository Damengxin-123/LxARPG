#pragma once

#include "CoreMinimal.h"
#include "LxAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillElementAbnormalSpec.h"
#include "LxARPG/LxSource/Player/Characters/LxCharacterStateEnum.h"
#include "LxElementAbnormalAttachSkillUnitActor.generated.h"

class UNiagaraComponent;
class ULxCharacterBuffModule;
class ULxCharacterStateAttributeObject;

/** 消费前置命中结果的元素异常依附单元：内部判定概率，成功维持效果，失败立即销毁。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="元素异常依附技能单元")
class LXARPG_API ALxElementAbnormalAttachSkillUnitActor : public ALxAttachEffectSkillUnitActor
{
	GENERATED_BODY()
public:
	/** 创建默认关闭的异常视觉组件并绑定统一清理入口。 */
	ALxElementAbnormalAttachSkillUnitActor();

	/** 同步异常参数，客户端仅播放已生效单元的表现。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 激活前设置异常参数；每个实例只能尝试生效一次。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|元素异常", DisplayName="初始化元素异常参数")
	void InitializeElementAbnormalParameters(const FLxSkillElementAbnormalSpec& InSpec);

protected:
	/** 目标来自前置命中结果，不检测自身表现组件的重叠。 */
	virtual bool RequiresOverlapEventSources() const override { return false; }

	/** 服务端仅尝试一次，客户端复现表现而不进行概率判定。 */
	virtual void ActivateSkillUnit_Implementation() override;

	/** 将异常伤害周期传给现有生命周期组件。 */
	virtual void ApplySkillUnitSpecToComponents() override;

	/** 订阅生命周期周期事件，不重复广播技能图的命中出口。 */
	virtual void BindSkillUnitComponentEvents() override;

	/** 到期先结算恰好落在终点的周期，再交给依附基类结束。 */
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;

	/** 检查角色和配置后判定触发几率；不包含异常免疫。 */
	virtual bool CanActivateAttachEffect() const override;

	/** 使用本单元作为独立来源，经目标效果传递模块施加状态和 Buff。 */
	virtual void HandleAttachEffectActivated() override;

	/** 只输出本次锁定目标的一次成功命中，避免表现碰撞体产生额外目标。 */
	virtual void HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult) override;

	/** 外部直接销毁或世界退出时也必须撤回效果。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 异常参数同步后补建粒子表现，兼容激活属性与资源的同步先后差异。 */
	UFUNCTION(Category="技能单元|元素异常", DisplayName="异常参数同步")
	void OnRep_AbnormalSpec();

	/** 随目标光环锚点移动的异常粒子组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|元素异常", DisplayName="异常视觉组件")
	TObjectPtr<UNiagaraComponent> AbnormalVisual;

	/** 本实例使用的异常配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AbnormalSpec, Category="技能单元|元素异常", DisplayName="异常参数")
	FLxSkillElementAbnormalSpec AbnormalSpec;

private:
	/** 消费已到期的伤害周期；同帧重入、取消或目标死亡不会重复结算。 */
	void HandlePeriodicDamage(float RemainingTime);

	/** 经施法者输出计算和目标效果接收模块施加一次伤害。 */
	void ApplyPeriodicDamage();

	/** 下次伤害结算的世界时间，小于零表示未启动或已清理。 */
	double NextDamageWorldTime = -1.0;

	/** 有限持续时间的最后有效伤害时刻，小于零表示无限持续。 */
	double DamageEndWorldTime = -1.0;

	/** 防止伤害计算事件回调重入周期处理。 */
	bool bProcessingPeriodicDamage = false;

	/** 依附正常结束、取消或到期时撤回所有贡献。 */
	void HandleAbnormalEnded(ALxAttachEffectSkillUnitActor* Unit, const FLxAttachEffectEndResult& Result);

	/** 目标死亡时结束异常，避免尸体持续持有减益。 */
	void HandleTargetStateChanged(ELxCharacterState State);

	/** 幂等释放本实例持有的状态、Buff 和视觉效果。 */
	void ReleaseAbnormalEffects();

	/** 本实例的稳定来源信息，销毁回调仍能据此准确撤回。 */
	UPROPERTY(Transient, VisibleInstanceOnly, meta=(DisplayName="异常效果来源", Category="技能单元|元素异常"))
	FLxEffectSourceContext AppliedSource;

	/** 保留目标状态组件的弱引用，便于目标结束生命周期时清理。 */
	TWeakObjectPtr<ULxCharacterStateAttributeObject> AppliedStateComponent;

	/** 保留目标 Buff 模块的弱引用，清理不依赖释放者仍然存在。 */
	TWeakObjectPtr<ULxCharacterBuffModule> AppliedBuffModule;

	/** 服务端已经执行过本实例的首次激活尝试。 */
	bool bActivationAttempted = false;

	/** 标记本实例是否需要撤回已提交的效果。 */
	bool bOwnsEffects = false;

	/** 成功施加后已经输出过命中结果。 */
	bool bPublishedAbnormalHit = false;
};
