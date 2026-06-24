#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMovementSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxSkillMovementComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillMovementStateChanged, ELxSkillAbilityComponentState, OldState, ELxSkillAbilityComponentState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillMovementProgress, float, MovementProgress);

/** 技能运动能力组件，只负责控制技能单元或指定场景组件的位移和运动状态。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能运动能力组件")
class LXARPG_API ULxSkillMovementComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxSkillMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 传入本次释放使用的运动参数，配置单位为 m、m/s、m/s²。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|运动", DisplayName="传入运动参数")
	void SetMovementSpec(const FLxSkillMovementSpec& InMovementSpec);

	/** 传入需要被此组件控制移动的场景组件；为空时默认控制Owner根组件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|运动", DisplayName="传入运动系统组件")
	void SetMovementTargetComponent(USceneComponent* InMovementTargetComponent);

	/** 开始运动。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|运动", DisplayName="开始运动")
	void StartMovement();

	/** 暂停运动。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|运动", DisplayName="暂停运动")
	void PauseMovement();

	/** 停止运动。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|运动", DisplayName="停止运动")
	void StopMovement();

	/** 获取运动阶段，0 表示刚开始，1 表示达到最大距离。 */
	UFUNCTION(BlueprintPure, Category="技能单元|运动", DisplayName="获取运动阶段")
	float GetMovementProgress() const;

	/** 获取当前运动状态。 */
	UFUNCTION(BlueprintPure, Category="技能单元|运动", DisplayName="获取运动状态")
	ELxSkillAbilityComponentState GetMovementState() const { return MovementState; }

	/** 运动状态改变事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|运动", DisplayName="运动状态改变事件")
	FOnLxSkillMovementStateChanged OnMovementStateChanged;

	/** 达到最大运动距离事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|运动", DisplayName="达到最大运动距离事件")
	FOnLxSkillMovementProgress OnReachMaxDistance;

protected:
	void SetMovementState(ELxSkillAbilityComponentState NewState);
	USceneComponent* ResolveMovementTargetComponent() const;

private:
	/** 运动参数，配置单位为 m、m/s、m/s²，组件运行时会转换为 UE 世界单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|运动", DisplayName="运动参数", meta=(AllowPrivateAccess="true"))
	FLxSkillMovementSpec MovementSpec;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> MovementTargetComponent = nullptr;

	ELxSkillAbilityComponentState MovementState = ELxSkillAbilityComponentState::Ready;
	float TraveledDistanceCm = 0.0f;
	float CurrentSpeedCmPerSecond = 0.0f;
};
