#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAttachSpec.h"
#include "LxAttachSkillUnitActor.generated.h"

class ULxSkillLifeComponent;
class ULxSkillTriggerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAttachPeriodEvent, ALxSkillUnitActor*, SkillUnit, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAttachTargetInvalidEvent, ALxSkillUnitActor*, SkillUnit, AActor*, AttachTarget);

/** 附着技能单元类型，负责协调附着目标、生命周期和周期触发。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="附着技能单元")
class LXARPG_API ALxAttachSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxAttachSkillUnitActor();

	/** 初始化附着参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|附着", DisplayName="初始化附着参数")
	void InitializeAttachParameters(const FLxSkillAttachSpec& InAttachSpec);

	/** 设置附着目标。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|附着", DisplayName="设置附着目标")
	void SetAttachTarget(AActor* InAttachTarget);

	UPROPERTY(BlueprintAssignable, Category="技能单元|附着", DisplayName="周期触发")
	FOnLxAttachPeriodEvent OnAttachPeriodTriggered;

	UPROPERTY(BlueprintAssignable, Category="技能单元|附着", DisplayName="附着失效")
	FOnLxSkillUnitResultEvent OnAttachExpired;

	UPROPERTY(BlueprintAssignable, Category="技能单元|附着", DisplayName="附着目标失效")
	FOnLxAttachTargetInvalidEvent OnAttachTargetInvalid;

protected:
	virtual void BindSkillUnitComponentEvents() override;
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;

	UFUNCTION()
	void HandleAttachLifeTick(float RemainingTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="触发能力组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|附着", DisplayName="附着参数")
	FLxSkillAttachSpec AttachSpec;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|附着", DisplayName="附着目标")
	TObjectPtr<AActor> AttachTarget = nullptr;
};
