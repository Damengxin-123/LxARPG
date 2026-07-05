#pragma once

#include "CoreMinimal.h"
#include "LxAreaSkillUnitActor.h"
#include "LxDurationAreaSkillUnitActor.generated.h"

/** 持续型范围效果，在持续时间内按照触发间隔反复命中当前范围内的目标。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="持续型范围效果技能子单元")
class LXARPG_API ALxDurationAreaSkillUnitActor : public ALxAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 初始化持续型范围效果。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|范围效果|持续型", DisplayName="初始化持续型范围效果")
	void InitializeDurationAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec,
		const FLxSkillDurationAreaEffectSpec& InDurationAreaEffectSpec);

protected:
	virtual void ApplySkillUnitSpecToComponents() override;
	virtual void BindSkillUnitComponentEvents() override;
	virtual bool ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const override;

	UFUNCTION()
	void HandleAreaDetectionPeriod(float RemainingTime);

	/** 持续型范围效果参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围效果|持续型", DisplayName="持续型参数")
	FLxSkillDurationAreaEffectSpec DurationAreaEffectSpec;
};
