#pragma once

#include "CoreMinimal.h"
#include "LxAreaSkillUnitActor.h"
#include "LxScalingAreaSkillUnitActor.generated.h"

/** 缩放型范围效果，在持续时间内匀速缩放判定范围，并命中所有进入范围的目标。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="缩放型范围效果技能子单元")
class LXARPG_API ALxScalingAreaSkillUnitActor : public ALxAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxScalingAreaSkillUnitActor();

	/** 初始化缩放型范围效果。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|范围效果|缩放型", DisplayName="初始化缩放型范围效果")
	void InitializeScalingAreaEffect(const FLxSkillAreaEffectSpec& InAreaEffectSpec,
		const FLxSkillScalingAreaEffectSpec& InScalingAreaEffectSpec);

	virtual void Tick(float DeltaSeconds) override;
	virtual void ActivateSkillUnit_Implementation() override;

protected:
	virtual bool ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const override;

	/** 缩放型范围效果参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|范围效果|缩放型", DisplayName="缩放型参数")
	FLxSkillScalingAreaEffectSpec ScalingAreaEffectSpec;

private:
	/** 激活时范围技能单元 Actor 的初始缩放。 */
	FVector InitialActorScale = FVector::OneVector;
	float ScalingStartWorldTime = -1.0f;
};
