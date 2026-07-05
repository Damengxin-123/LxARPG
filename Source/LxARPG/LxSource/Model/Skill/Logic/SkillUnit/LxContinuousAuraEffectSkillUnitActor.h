#pragma once

#include "CoreMinimal.h"
#include "LxAuraEffectSkillUnitActor.h"
#include "LxContinuousAuraEffectSkillUnitActor.generated.h"

class ALxContinuousAuraEffectSkillUnitActor;

/** 持续型光环回收目标效果事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxAuraTargetEffectRemoved, ALxContinuousAuraEffectSkillUnitActor*, SkillUnit, const FLxAuraTargetEffectRemoveResult&, RemoveResult);

/** 持续型光环，在目标进入时命中，并在离开或光环停用时回收对应效果。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="持续型光环效果单元")
class LXARPG_API ALxContinuousAuraEffectSkillUnitActor : public ALxAuraEffectSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 获取当前仍由光环维持效果的目标数组。 */
	UFUNCTION(BlueprintPure, Category="技能单元|光环效果|持续型", DisplayName="获取当前生效目标")
	TArray<AActor*> GetActiveAuraTargets() const;

	/** 单个目标的持续光环效果需要被回收时广播。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|光环效果|持续型|事件", DisplayName="光环目标效果回收事件")
	FOnLxAuraTargetEffectRemoved OnAuraTargetEffectRemoved;

protected:
	/** 启用后主动扫描当前已经位于范围内的目标。 */
	virtual void HandleAuraEffectActivated() override;

	/** 停用前回收所有仍在生效的目标效果。 */
	virtual void HandleAuraEffectDeactivated(ELxAuraTargetEffectRemoveReason RemoveReason) override;

	/** 根据进入、离开和启动扫描结果维护生效目标集合。 */
	virtual void HandleAuraDetectionResult(const FLxSkillDetectionResult& DetectionResult) override;

	/** 目标Actor被销毁时移除运行状态并广播目标失效。 */
	UFUNCTION()
	void HandleActiveAuraTargetDestroyed(AActor* DestroyedActor);

private:
	/** 将有效目标加入持续生效集合并广播一次标准命中。 */
	void AddActiveAuraTarget(AActor* TargetActor);

	/** 从集合移除目标并广播效果回收事件。 */
	void RemoveActiveAuraTarget(AActor* TargetActor, ELxAuraTargetEffectRemoveReason RemoveReason);

	/** 当前仍由本光环维持效果的目标集合。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> ActiveAuraTargets;
};