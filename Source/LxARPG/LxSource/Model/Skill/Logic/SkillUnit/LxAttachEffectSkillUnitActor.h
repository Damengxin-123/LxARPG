#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAttachEffectSpec.h"
#include "LxAttachEffectSkillUnitActor.generated.h"

class ULxSkillLifeComponent;
class ULxSkillTriggerComponent;
class USceneComponent;
class ALxAttachEffectSkillUnitActor;

/** 依附效果结束事件；技能单元对象本身可作为本次持续效果的唯一来源标识。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAttachEffectEnded, ALxAttachEffectSkillUnitActor*, SkillUnit, const FLxAttachEffectEndResult&, EndResult);

/**
 * 依附效果技能单元基类，只负责消费前置命中结果、依附目标和管理生命周期。
 * 本类型不能作为技能链首个单元，也不直接创建对象，具体触发规则由子类实现。
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DisplayName="依附效果技能单元基类")
class LXARPG_API ALxAttachEffectSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 创建依附效果技能单元基类。 */
	ALxAttachEffectSkillUnitActor();

	/** 从前置技能单元命中结果的指定目标初始化依附关系。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|依附效果", DisplayName="使用前置命中结果初始化依附效果")
	bool InitializeFromPreviousSkillUnitResult(const FLxSkillUnitResult& PreviousResult, int32 HitTargetIndex,
		const FLxSkillAttachEffectSpec& InAttachEffectSpec);

	/** 获取当前依附目标。 */
	UFUNCTION(BlueprintPure, Category="技能单元|依附效果", DisplayName="获取依附目标")
	AActor* GetAttachTarget() const { return AttachTarget; }

	/** 获取提供目标的前置技能单元。 */
	UFUNCTION(BlueprintPure, Category="技能单元|依附效果", DisplayName="获取前置技能单元")
	UObject* GetPreviousSkillUnit() const { return PreviousSkillUnit; }

	/** 依附结束事件；持续生效子类应在此事件中撤销本单元施加的效果。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|依附效果|事件", DisplayName="依附效果结束事件")
	FOnLxAttachEffectEnded OnAttachEffectEnded;

protected:
	/** 校验前置结果和目标后完成依附并启动生命周期。 */
	virtual void ActivateSkillUnit_Implementation() override;

	/** 主动停止依附效果并按正常结束流程销毁。 */
	virtual void StopSkillUnit_Implementation() override;

	/** 异常取消依附效果并按取消流程销毁。 */
	virtual void CancelSkillUnit_Implementation() override;

	/** 将依附持续时间写入生命周期组件。 */
	virtual void ApplySkillUnitSpecToComponents() override;

	/** 生命周期到期时结束依附效果。 */
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;

	/** 检查子类型独有参数是否允许激活。 */
	virtual bool CanActivateAttachEffect() const;

	/** 完成依附并启动组件后执行子类型触发逻辑。 */
	virtual void HandleAttachEffectActivated();

	/** 为当前依附目标广播一次标准技能单元命中。 */
	void TriggerAttachTargetHit();

	/** 结束依附、广播结束数据并销毁技能单元。 */
	void EndAttachEffect(ELxAttachEffectEndReason EndReason, ELxSkillUnitResultType ResultType, bool bSuccess);

	/** 根据目标和插槽配置解析实际依附组件。 */
	USceneComponent* ResolveAttachComponent() const;

	/** 目标Actor被销毁时终止依附效果。 */
	UFUNCTION()
	void HandleAttachTargetDestroyed(AActor* DestroyedActor);

	/** 依附效果生命周期组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|依附效果|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	/** 将依附命中转发为通用技能命中事件的触发组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|依附效果|组件", DisplayName="命中触发组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	/** 依附效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果", DisplayName="依附效果参数")
	FLxSkillAttachEffectSpec AttachEffectSpec;

	/** 从前置技能单元命中结果中解析出的唯一依附目标。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|依附效果|运行状态", DisplayName="依附目标")
	TObjectPtr<AActor> AttachTarget = nullptr;

	/** 提供当前依附目标的前置技能单元。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|依附效果|运行状态", DisplayName="前置技能单元")
	TObjectPtr<UObject> PreviousSkillUnit = nullptr;

	/** 当前依附是否已经成功广播过至少一次命中。 */
	bool bAttachEffectApplied = false;

private:
	bool bEndingAttachEffect = false;
};