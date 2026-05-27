#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAreaSpec.h"
#include "LxAreaSkillUnitActor.generated.h"

class USphereComponent;
class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillTriggerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAreaDetectionEvent, ALxSkillUnitActor*, SkillUnit, const FLxSkillDetectionResult&, DetectionResult);

/** 范围技能单元类型，负责协调范围碰撞、检测、触发和生命周期组件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="范围技能单元")
class LXARPG_API ALxAreaSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxAreaSkillUnitActor();

	/** 初始化范围参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|范围", DisplayName="初始化范围参数")
	void InitializeAreaParameters(const FLxSkillAreaSpec& InAreaSpec);

	UPROPERTY(BlueprintAssignable, Category="技能单元|范围", DisplayName="目标进入范围")
	FOnLxAreaDetectionEvent OnTargetEnterArea;

	UPROPERTY(BlueprintAssignable, Category="技能单元|范围", DisplayName="目标离开范围")
	FOnLxAreaDetectionEvent OnTargetLeaveArea;

protected:
	virtual void InitializeSkillUnitDefaultParameters_Implementation() override;
	virtual void ApplySkillUnitSpecToComponents() override;
	virtual void BindSkillUnitComponentEvents() override;

	UFUNCTION()
	virtual void HandleAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="范围碰撞体")
	TObjectPtr<USphereComponent> AreaCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="目标检测组件")
	TObjectPtr<ULxSkillDetectionComponent> DetectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="触发能力组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围", DisplayName="范围参数")
	FLxSkillAreaSpec AreaSpec;
};
