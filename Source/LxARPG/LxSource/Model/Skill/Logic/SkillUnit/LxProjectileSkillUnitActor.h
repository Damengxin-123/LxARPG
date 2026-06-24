#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillProjectileSpec.h"
#include "LxProjectileSkillUnitActor.generated.h"

class USphereComponent;
class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillMovementComponent;
class ALxProjectileSkillUnitActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxProjectileTriggered, ALxProjectileSkillUnitActor*, Projectile, const FLxProjectileTriggerContext&, TriggerContext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxProjectileInvalidated, ALxProjectileSkillUnitActor*, Projectile, const FLxProjectileInvalidationContext&, InvalidationContext);

/** 投射物技能单元基类，只负责飞行、命中检测、穿透计数和失效流程。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="投射物技能单元")
class LXARPG_API ALxProjectileSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxProjectileSkillUnitActor();

	/** 初始化投射物参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|投射物", DisplayName="初始化投射物参数")
	void InitializeProjectileParameters(const FLxSkillProjectileSpec& InProjectileSpec);

	/** 投射物命中有效目标时触发，技能可在这里接收目标并处理效果传递。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|投射物|事件", DisplayName="投射物触发行为")
	FOnLxProjectileTriggered OnProjectileTriggered;

	/** 投射物达到失效条件时触发。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|投射物|事件", DisplayName="投射物失效事件")
	FOnLxProjectileInvalidated OnProjectileInvalidated;

protected:
	virtual void ActivateSkillUnit_Implementation() override;
	virtual void InitializeSkillUnitDefaultParameters_Implementation() override;
	virtual void ApplySkillUnitSpecToComponents() override;
	virtual void BindSkillUnitComponentEvents() override;
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState) override;

	/** 处理投射物检测结果。 */
	UFUNCTION()
	void HandleProjectileDetectionResult(const FLxSkillDetectionResult& DetectionResult);

	/** 处理投射物达到最大飞行距离。 */
	UFUNCTION()
	void HandleProjectileReachMaxDistance(float MovementProgress);

	/** 重置投射物运行期命中状态。 */
	void ResetProjectileRuntimeState();

	/** 构造投射物触发行为上下文。 */
	FLxProjectileTriggerContext MakeProjectileTriggerContext(const FLxSkillDetectionResult& DetectionResult) const;

	/** 构造投射物失效上下文。 */
	FLxProjectileInvalidationContext MakeProjectileInvalidationContext(const FTransform& InvalidationTransform) const;

	/** 按指定位置使投射物失效。 */
	void InvalidateProjectile(const FTransform& InvalidationTransform);

	/** 从检测结果提取后续子单元可使用的创建变换。 */
	FTransform MakeSpawnTransformFromDetectionResult(const FLxSkillDetectionResult& DetectionResult) const;

	/** 判断目标是否已经被该投射物触发过。 */
	bool HasTriggeredTarget(AActor* InTarget) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="碰撞体")
	TObjectPtr<USphereComponent> ProjectileCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="运动能力组件")
	TObjectPtr<ULxSkillMovementComponent> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="目标检测组件")
	TObjectPtr<ULxSkillDetectionComponent> DetectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物", DisplayName="投射物参数")
	FLxSkillProjectileSpec ProjectileSpec;

	/** 剩余可穿透角色目标数量。 */
	int32 RemainingPierceCount = 0;

	/** 是否已经进入失效流程。 */
	bool bProjectileInvalidated = false;

	/** 已经触发过的有效目标列表。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> TriggeredTargets;
};
