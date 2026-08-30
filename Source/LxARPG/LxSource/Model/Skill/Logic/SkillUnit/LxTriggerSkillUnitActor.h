#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillTriggerUnitSpec.h"
#include "LxTriggerSkillUnitActor.generated.h"

class ULxSkillLifeComponent;
class ULxSkillTriggerComponent;

/** 触发器技能单元类型，负责协调碰撞检测、触发判断和生命周期。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="触发器技能单元")
class LXARPG_API ALxTriggerSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxTriggerSkillUnitActor();

	/** 初始化触发器参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|触发器", DisplayName="初始化触发器参数")
	void InitializeTriggerUnitParameters(const FLxSkillTriggerUnitSpec& InTriggerUnitSpec);

	UPROPERTY(BlueprintAssignable, Category="技能单元|触发器", DisplayName="触发器被触发")
	FOnLxSkillUnitTriggerEvent OnTriggerUnitTriggered;

	UPROPERTY(BlueprintAssignable, Category="技能单元|触发器", DisplayName="触发器失效")
	FOnLxSkillUnitResultEvent OnTriggerUnitExpired;

protected:
	virtual void InitializeSkillUnitDefaultParameters_Implementation() override;
	virtual void ApplySkillUnitSpecToComponents() override;
	virtual void HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult) override;
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="触发能力组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发器", DisplayName="触发器参数")
	FLxSkillTriggerUnitSpec TriggerUnitSpec;
};
