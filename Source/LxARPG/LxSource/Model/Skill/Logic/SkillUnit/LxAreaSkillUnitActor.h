#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAreaSpec.h"
#include "LxAreaSkillUnitActor.generated.h"

class UPrimitiveComponent;
class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillTriggerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAreaDetectionEvent, ALxSkillUnitActor*, SkillUnit, const FLxSkillDetectionResult&, DetectionResult);

/** 范围效果技能子单元基类，负责协调外部判定组件、目标检测、触发和持续时间。 */
UCLASS(Abstract, Blueprintable, BlueprintType, DisplayName="范围效果技能子单元基类")
class LXARPG_API ALxAreaSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxAreaSkillUnitActor();

	/** 初始化范围效果，并自动收集当前技能单元对象树上的全部碰撞体。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|范围效果", DisplayName="初始化范围效果")
	void InitializeAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec);

	/** 目标进入范围时发布的检测事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|范围效果|事件", DisplayName="目标进入范围")
	FOnLxAreaDetectionEvent OnTargetEnterArea;

	/** 目标离开范围时发布的检测事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|范围效果|事件", DisplayName="目标离开范围")
	FOnLxAreaDetectionEvent OnTargetLeaveArea;

protected:
	virtual void ApplySkillUnitSpecToComponents() override;
	virtual void BindSkillUnitComponentEvents() override;
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;
	virtual bool ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const;

	/** 对判定组件当前覆盖的全部对象执行一次手动检测。 */
	void ScanCurrentAreaTargets();

	/** 返回当前使用的全部范围判定组件。 */
	TArray<UPrimitiveComponent*> GetAreaDetectionCollisionComponents() const;

	/** 从范围技能单元自身对象树中解析全部用于目标检测的碰撞体。 */
	TArray<UPrimitiveComponent*> ResolveAreaDetectionCollisionComponents() const;

	UFUNCTION()
	virtual void HandleAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult);

	/** 目标检测组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|范围效果|组件", DisplayName="目标检测组件")
	TObjectPtr<ULxSkillDetectionComponent> DetectionComponent;

	/** 命中触发组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|范围效果|组件", DisplayName="命中触发组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	/** 生命周期组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|范围效果|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	/** 范围效果共有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围效果", DisplayName="范围效果参数")
	FLxSkillAreaEffectSpec AreaEffectSpec;

	/** 当前范围技能单元自身对象树中用于检测的全部碰撞体。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能单元|范围效果|组件", DisplayName="范围判定碰撞体")
	TArray<TObjectPtr<UPrimitiveComponent>> DetectionCollisionComponents;
};
