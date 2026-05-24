#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillProjectileSpec.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillMovementComponent.h"
#include "LxProjectileSkillUnitActor.generated.h"

class USphereComponent;
class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillMovementComponent;
class ULxSkillPropagationComponent;
class ULxSkillTriggerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxProjectileDetectionEvent, const FLxSkillDetectionResult&, DetectionResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxProjectilePropagationEvent, const FLxSkillPropagationResult&, PropagationResult);

/** 投射物技能单元类型，负责协调运动、检测、触发、传播和生命周期组件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="投射物技能单元")
class LXARPG_API ALxProjectileSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxProjectileSkillUnitActor();

	/** 初始化投射物参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|投射物", DisplayName="初始化投射物参数")
	void InitializeProjectileParameters(const FLxSkillProjectileSpec& InProjectileSpec);

	UPROPERTY(BlueprintAssignable, Category="技能单元|投射物", DisplayName="投射物命中目标")
	FOnLxProjectileDetectionEvent OnProjectileHitTarget;

	UPROPERTY(BlueprintAssignable, Category="技能单元|投射物", DisplayName="投射物穿透目标")
	FOnLxProjectilePropagationEvent OnProjectilePierceTarget;

	UPROPERTY(BlueprintAssignable, Category="技能单元|投射物", DisplayName="投射物达最大飞行距离")
	FOnLxSkillMovementProgress OnProjectileReachMaxDistance;

	UPROPERTY(BlueprintAssignable, Category="技能单元|投射物", DisplayName="投射物碰撞到障碍物")
	FOnLxProjectileDetectionEvent OnProjectileHitWorld;

protected:
	virtual void InitializeSkillUnitDefaultParameters_Implementation() override;
	virtual void ApplySkillUnitSpecToComponents() override;
	virtual void BindSkillUnitComponentEvents() override;

	UFUNCTION()
	void HandleProjectileDetectionResult(const FLxSkillDetectionResult& DetectionResult);

	UFUNCTION()
	void HandleProjectilePropagationResult(const FLxSkillPropagationResult& PropagationResult);

	UFUNCTION()
	void HandleProjectileReachMaxDistance(float MovementProgress);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="碰撞体")
	TObjectPtr<USphereComponent> ProjectileCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="运动能力组件")
	TObjectPtr<ULxSkillMovementComponent> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="目标检测组件")
	TObjectPtr<ULxSkillDetectionComponent> DetectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="触发能力组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="传播能力组件")
	TObjectPtr<ULxSkillPropagationComponent> PropagationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物", DisplayName="投射物参数")
	FLxSkillProjectileSpec ProjectileSpec;
};
