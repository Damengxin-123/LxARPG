#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxCharacterSpecialAttributeObject.h"
#include "LxCharacterLifecycleAttributeObject.generated.h"

/** 角色生命周期特殊属性业务对象，负责死亡与复活时的移动控制。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色生命周期特殊属性")
class LXARPG_API ULxCharacterLifecycleAttributeObject : public ULxCharacterSpecialAttributeObject
{
	GENERATED_BODY()

public:
	/** 绑定角色属性变化事件并检查初始生命值。 */
	virtual void InitializeSpecialAttributeObject(ULxCharacterAttributeComponent* InOwnerComponent) override;

	/** 解除角色属性变化事件。 */
	virtual void DeinitializeSpecialAttributeObject() override;

	/** 根据当前存活状态应用移动控制、倒地动画和延迟销毁逻辑。 */
	void ApplyLifecycleState(bool bInAlive);

	/** 判断角色当前是否存活。 */
	bool IsCharacterAlive() const { return bIsAlive; }

	/** 设置角色当前存活状态，并通知统一属性组件。 */
	void SetCharacterAliveState(bool bInAlive);

	/** 注册存活状态网络复制字段。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 获取存活状态标签。 */
	FGameplayTag GetAliveStateTag() const { return AliveStateTag; }

	/** 获取死亡状态标签。 */
	FGameplayTag GetDeadStateTag() const { return DeadStateTag; }

protected:
	/** 死亡时是否禁用角色移动。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="死亡时禁用移动")
	bool bDisableMovementWhenDead = true;

	/** 角色死亡后是否自动销毁角色对象。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="死亡后销毁角色")
	bool bDestroyCharacterWhenDead = true;

	/** 角色死亡后等待多少秒销毁角色对象。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="死亡后销毁延迟", meta=(ClampMin="0.0", UIMin="0.0", Units="s"))
	float DeathDestroyDelay = 2.0f;

	/** 角色进入死亡状态时发送给动画系统的倒地动画类型。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="倒地动画类型")
	ELxCharacterMotionType DeathAnimationType = ELxCharacterMotionType::Dead;

	/** 存活状态标签。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="存活状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTag AliveStateTag;

	/** 死亡状态标签。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="死亡状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTag DeadStateTag;

private:
	/** 存活状态复制回调。 */
	UFUNCTION(Category="角色|属性|生命周期|网络", DisplayName="同步角色存活状态")
	void OnRep_IsAlive();

	/** 角色属性变化时检查生命值是否已经归零。 */
	UFUNCTION(Category="角色|特殊属性|生命周期", DisplayName="处理生命周期属性变化")
	void HandleCharacterAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot);

	/** 在权威端根据指定属性快照检查并触发死亡。 */
	void EvaluateDeathFromAttributeSnapshot(const FLxTypedAttributeSnapshot& AttributeSnapshot) const;

	/** 根据当前运行时属性检查并触发死亡。 */
	void EvaluateDeathFromCurrentAttributes() const;

	/** 对死亡角色播放倒地动画，并由权威端启动销毁倒计时。 */
	void StartDeathSequence();

	/** 当前死亡表现和销毁倒计时是否已经启动。 */
	bool bDeathSequenceStarted = false;

	/** 当前角色是否存活。 */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_IsAlive, Category="角色|属性|生命周期", DisplayName="角色是否存活")
	bool bIsAlive = true;

public:
	/** 创建生命周期特殊属性并设置默认状态标签。 */
	ULxCharacterLifecycleAttributeObject();
};
