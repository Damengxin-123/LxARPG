#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillRaySpec.h"
#include "LxSkillUnitActor.h"
#include "LxRaySkillUnitActor.generated.h"

class ULxSkillPropagationComponent;
class ULxSkillTriggerComponent;
class UPrimitiveComponent;
class ALxRaySkillUnitActor;

/** 射线检测完成事件，无论是否命中目标都会返回本次检测结果。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxRayDetectionCompleted, ALxRaySkillUnitActor*, RaySkillUnit, const FLxSkillDetectionResult&, DetectionResult);

/** 抽象射线效果单元，统一执行直线检测和穿透规则，不直接用于创建对象。 */
UCLASS(Abstract, Blueprintable, BlueprintType, DisplayName="射线效果单元基类")
class LXARPG_API ALxRaySkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxRaySkillUnitActor();

	/** 初始化射线判定体，自动收集当前对象树上已启用的胶囊碰撞组件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|射线", DisplayName="初始化射线判定碰撞体")
	void InitializeRayDetectionCollisionComponents();

	/** 初始化射线穿过目标和障碍物的公共命中规则。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|射线", DisplayName="初始化射线公共参数")
	void InitializeRayParameters(const FLxSkillRaySpec& InRaySpec);

	/** 每次射线检测完成时广播，未命中时也会广播。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|射线|事件", DisplayName="射线检测完成事件")
	FOnLxRayDetectionCompleted OnRayDetectionCompleted;

protected:
	/** 扫描当前射线胶囊判定体内的目标，并转交触发组件。 */
	FLxSkillDetectionResult PerformRayDetection();

	/** 从当前技能单元对象树上解析全部已启用的胶囊判定体。 */
	TArray<UPrimitiveComponent*> ResolveRayDetectionCollisionComponents() const;

	/** 判断命中对象是否属于射线可以返回的有效目标。 */
	virtual bool IsValidRayTarget(AActor* InActor) const;

	/** 检查射线起点到目标之间是否存在阻挡可见性通道的场景障碍物。 */
	bool FindObstacleBeforeTarget(AActor* TargetActor, const TArray<AActor*>& RayTargets,
		FHitResult& OutObstacleHit) const;

	/** 射线最终触发后继续广播通用技能单元事件。 */
	virtual void HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="触发能力组件")
	TObjectPtr<ULxSkillTriggerComponent> TriggerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="传播能力组件")
	TObjectPtr<ULxSkillPropagationComponent> PropagationComponent;

	/** 当前射线单元对象树上用于命中扫描的胶囊判定体。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能单元|射线|组件", DisplayName="射线判定碰撞体")
	TArray<TObjectPtr<UPrimitiveComponent>> RayDetectionCollisionComponents;

	/** 当前射线穿过目标和障碍物的公共规则。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|射线", DisplayName="射线公共参数")
	FLxSkillRaySpec RaySpec;
};
