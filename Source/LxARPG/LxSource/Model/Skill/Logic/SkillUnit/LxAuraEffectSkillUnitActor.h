#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAuraEffectSpec.h"
#include "LxAuraEffectSkillUnitActor.generated.h"

class ALxBaseCharacter;
class UPrimitiveComponent;
class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillTriggerComponent;

/**
 * 光环效果技能单元基类，只依附释放者并协调蓝图范围碰撞体、检测、触发和可重复启停生命周期。
 * 本类型不直接创建对象，具体命中规则由持续型和周期型子类实现。
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DisplayName="光环效果技能单元基类")
class LXARPG_API ALxAuraEffectSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 创建光环效果技能单元基类和通用能力组件。 */
	ALxAuraEffectSkillUnitActor();

	/** 将光环固定依附到释放者的光环效果锚点并写入共有参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|光环效果", DisplayName="初始化光环效果")
	bool InitializeAuraEffect(ALxBaseCharacter* InAuraOwner, const FLxSkillAuraEffectSpec& InAuraEffectSpec);

	/** 获取当前光环固定依附的释放者。 */
	UFUNCTION(BlueprintPure, Category="技能单元|光环效果", DisplayName="获取光环释放者")
	ALxBaseCharacter* GetAuraOwner() const { return AuraOwner; }

	/**
	 * 返回蓝图中手动创建并调整范围的光环碰撞体。
	 * 默认返回本Actor找到的第一个图元组件，存在多个图元组件时应在蓝图中覆写并明确返回目标组件。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元|光环效果", DisplayName="解析光环检测碰撞体")
	UPrimitiveComponent* ResolveAuraDetectionComponent() const;
	virtual UPrimitiveComponent* ResolveAuraDetectionComponent_Implementation() const;

protected:
	/** 结束播放时解除释放者销毁事件绑定。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 解析范围碰撞体并启动一次可复用的光环运行周期。 */
	virtual void ActivateSkillUnit_Implementation() override;

	/** 主动停止当前光环周期，但保留技能单元对象。 */
	virtual void StopSkillUnit_Implementation() override;

	/** 异常取消当前光环周期，但保留技能单元对象。 */
	virtual void CancelSkillUnit_Implementation() override;

	/** 将光环持续时间分发给生命周期组件。 */
	virtual void ApplySkillUnitSpecToComponents() override;

	/** 绑定光环范围检测结果。 */
	virtual void BindSkillUnitComponentEvents() override;

	/** 光环持续时间结束时停止当前周期而不销毁Actor。 */
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;

	/** 检查共有参数和子类型参数是否允许启动光环。 */
	virtual bool CanActivateAuraEffect() const;

	/** 光环能力组件启动后执行子类型逻辑。 */
	virtual void HandleAuraEffectActivated();

	/** 光环停止前执行子类型目标清理逻辑。 */
	virtual void HandleAuraEffectDeactivated(ELxAuraTargetEffectRemoveReason RemoveReason);

	/** 接收检测组件发布的范围进入、离开或手动扫描结果。 */
	UFUNCTION()
	virtual void HandleAuraDetectionResult(const FLxSkillDetectionResult& DetectionResult);

	/** 扫描当前光环碰撞体内除释放者以外的全部有效角色目标。 */
	void ScanCurrentAuraTargets();

	/** 为单个目标广播一次标准技能单元命中。 */
	void TriggerAuraTargetHit(AActor* HitTarget);

	/** 结束当前光环启用周期并保留Actor供下次启用。 */
	void EndAuraActivation(ELxAuraTargetEffectRemoveReason RemoveReason, ELxSkillUnitResultType ResultType, bool bSuccess);

	/** 释放者被销毁时取消光环并销毁无法继续复用的单元对象。 */
	UFUNCTION()
	void HandleAuraOwnerDestroyed(AActor* DestroyedActor);

	/** 光环范围检测组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|光环效果|组件", DisplayName="目标检测组件")
	TObjectPtr<ULxSkillDetectionComponent> DetectionComponent;

	/** 光环命中触发组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|光环效果|组件", DisplayName="命中触发组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	/** 光环生命周期组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|光环效果|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	/** 光环效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|光环效果", DisplayName="光环效果参数")
	FLxSkillAuraEffectSpec AuraEffectSpec;

	/** 光环固定依附的释放者。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|光环效果|运行状态", DisplayName="光环释放者")
	TObjectPtr<ALxBaseCharacter> AuraOwner = nullptr;

	/** 本次启用使用的蓝图范围碰撞体。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能单元|光环效果|运行状态", DisplayName="光环检测碰撞体")
	TObjectPtr<UPrimitiveComponent> AuraDetectionComponent = nullptr;

private:
	bool bEndingAuraActivation = false;
};