#pragma once

#include "CoreMinimal.h"
#include "LxAuraEffectSkillUnitActor.h"
#include "LxPeriodicAuraEffectSkillUnitActor.generated.h"

/** 周期触发型光环，在启用期间按固定间隔扫描并命中范围内所有有效目标。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="周期触发型光环效果单元")
class LXARPG_API ALxPeriodicAuraEffectSkillUnitActor : public ALxAuraEffectSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 初始化周期光环独有参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|光环效果|周期型", DisplayName="初始化周期光环参数")
	void InitializePeriodicAuraEffectParameters(const FLxSkillPeriodicAuraEffectSpec& InPeriodicSpec);

protected:
	/** 将触发间隔写入生命周期周期时钟。 */
	virtual void ApplySkillUnitSpecToComponents() override;

	/** 绑定生命周期周期事件。 */
	virtual void BindSkillUnitComponentEvents() override;

	/** 同时检查持续时间和周期触发间隔。 */
	virtual bool CanActivateAuraEffect() const override;

	/** 根据配置决定启用后是否立即执行范围命中。 */
	virtual void HandleAuraEffectActivated() override;

	/** 仅处理周期扫描产生的检测结果。 */
	virtual void HandleAuraDetectionResult(const FLxSkillDetectionResult& DetectionResult) override;

	/** 周期时钟到达时扫描当前光环范围。 */
	UFUNCTION()
	void HandleAuraEffectPeriod(float RemainingTime);

	/** 周期触发型光环独有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|光环效果|周期型", DisplayName="周期光环参数")
	FLxSkillPeriodicAuraEffectSpec PeriodicSpec;
};