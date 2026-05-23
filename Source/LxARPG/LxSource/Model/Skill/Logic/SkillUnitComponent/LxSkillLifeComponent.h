#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillLifeSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxSkillLifeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillLifeStateChanged, ELxSkillAbilityComponentState, OldState, ELxSkillAbilityComponentState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillLifeTick, float, RemainingTime);

/** 技能生命周期组件，只负责持续时间、周期时钟和生命周期状态通知。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能生命周期组件")
class LXARPG_API ULxSkillLifeComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 传入生命周期参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|生命周期", DisplayName="传入生命周期参数")
	void SetLifeSpec(const FLxSkillLifeSpec& InLifeSpec);

	/** 设置生命周期周期事件的触发间隔；0 表示不触发周期事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|生命周期", DisplayName="设置周期触发间隔")
	void SetLifeTickInterval(float InLifeTickInterval);

	/** 开始运行生命周期计时。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|生命周期", DisplayName="开始运行")
	void StartLife();

	/** 停止生命周期计时。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|生命周期", DisplayName="停止运行")
	void StopLife();

	/** 获取剩余持续时间；小于 0 表示无限或未启动。 */
	UFUNCTION(BlueprintPure, Category="技能单元|生命周期", DisplayName="返回剩余持续时间")
	float GetRemainingDuration() const;

	/** 生命周期状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|生命周期", DisplayName="生命周期发生变化")
	FOnLxSkillLifeStateChanged OnLifeStateChanged;

	/** 生命周期周期事件，只表达时间到达，不直接代表命中或效果触发。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|生命周期", DisplayName="周期触发事件")
	FOnLxSkillLifeTick OnLifeTick;

private:
	void SetLifeState(ELxSkillAbilityComponentState NewState);

	UFUNCTION()
	void HandleLifeExpired();

	UFUNCTION()
	void HandleLifeTick();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|生命周期", DisplayName="生命周期参数", meta=(AllowPrivateAccess="true"))
	FLxSkillLifeSpec LifeSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|生命周期", DisplayName="生命周期周期间隔", meta=(AllowPrivateAccess="true"))
	float LifeTickInterval = 0.0f;

	FTimerHandle LifeTimerHandle;
	FTimerHandle LifeTickTimerHandle;
	ELxSkillAbilityComponentState LifeState = ELxSkillAbilityComponentState::Ready;
	float LifeStartWorldTime = -1.0f;
};
