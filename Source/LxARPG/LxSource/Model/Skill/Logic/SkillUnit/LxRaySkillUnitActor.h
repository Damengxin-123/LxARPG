#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillRaySpec.h"
#include "LxRaySkillUnitActor.generated.h"

class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillPropagationComponent;
class ULxSkillTriggerComponent;

/** 射线技能单元类型，负责协调检测、触发、传播和生命周期组件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="射线技能单元")
class LXARPG_API ALxRaySkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxRaySkillUnitActor();

	/** 初始化射线效果参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|射线", DisplayName="初始化射线效果参数")
	void InitializeRayParameters(const FLxSkillRaySpec& InRaySpec);

	/** 射线命中目标事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|射线", DisplayName="射线命中目标")
	FOnLxSkillUnitTriggerEvent OnRayHitTarget;

protected:
	virtual void HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="目标检测组件")
	TObjectPtr<ULxSkillDetectionComponent> DetectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="触发能力组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="传播能力组件")
	TObjectPtr<ULxSkillPropagationComponent> PropagationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|射线", DisplayName="射线参数")
	FLxSkillRaySpec RaySpec;
};
