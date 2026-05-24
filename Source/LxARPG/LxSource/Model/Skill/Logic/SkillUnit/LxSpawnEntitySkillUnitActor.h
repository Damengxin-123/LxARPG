#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillSpawnEntitySpec.h"
#include "LxSpawnEntitySkillUnitActor.generated.h"

class ULxSkillLifeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSpawnedEntityEvent, AActor*, EntityActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSpawnedEntityTargetEvent, AActor*, EntityActor, AActor*, TargetActor);

/** 召唤实体技能单元类型，负责协调生成实体的生命周期和事件转发。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="召唤实体技能单元")
class LXARPG_API ALxSpawnEntitySkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxSpawnEntitySkillUnitActor();

	/** 初始化召唤实体参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|召唤实体", DisplayName="初始化召唤实体参数")
	void InitializeSpawnEntityParameters(const FLxSkillSpawnEntitySpec& InSpawnEntitySpec);

	/** 注册一个已创建实体，并广播实体完成创建事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|召唤实体", DisplayName="注册已创建实体")
	void RegisterSpawnedEntity(AActor* InEntityActor);

	/** 通知实体被销毁。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|召唤实体", DisplayName="通知实体被销毁")
	void NotifySpawnedEntityDestroyed(AActor* InEntityActor);

	/** 通知实体命中目标。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|召唤实体", DisplayName="通知实体命中目标")
	void NotifySpawnedEntityHitTarget(AActor* InEntityActor, AActor* InTargetActor);

	/** 通知实体销毁目标。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|召唤实体", DisplayName="通知实体销毁目标")
	void NotifySpawnedEntityKilledTarget(AActor* InEntityActor, AActor* InTargetActor);

	UPROPERTY(BlueprintAssignable, Category="技能单元|召唤实体", DisplayName="实体完成创建")
	FOnLxSpawnedEntityEvent OnEntityCreated;

	UPROPERTY(BlueprintAssignable, Category="技能单元|召唤实体", DisplayName="实体被销毁")
	FOnLxSpawnedEntityEvent OnEntityDestroyed;

	UPROPERTY(BlueprintAssignable, Category="技能单元|召唤实体", DisplayName="实体命中目标")
	FOnLxSpawnedEntityTargetEvent OnEntityHitTarget;

	UPROPERTY(BlueprintAssignable, Category="技能单元|召唤实体", DisplayName="实体销毁目标")
	FOnLxSpawnedEntityTargetEvent OnEntityKilledTarget;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|组件", DisplayName="生命周期组件")
	TObjectPtr<ULxSkillLifeComponent> LifeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|召唤实体", DisplayName="召唤实体参数")
	FLxSkillSpawnEntitySpec SpawnEntitySpec;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="技能单元|召唤实体", DisplayName="已创建实体")
	TArray<TObjectPtr<AActor>> SpawnedEntities;
};
