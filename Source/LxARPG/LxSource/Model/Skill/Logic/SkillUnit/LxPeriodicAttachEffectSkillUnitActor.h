#pragma once

#include "CoreMinimal.h"
#include "LxAttachEffectSkillUnitActor.h"
#include "LxPeriodicAttachEffectSkillUnitActor.generated.h"

/** 周期触发依附单元，在有效持续时间内按固定间隔重复广播目标命中。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="周期触发依附效果单元")
class LXARPG_API ALxPeriodicAttachEffectSkillUnitActor : public ALxAttachEffectSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 初始化周期依附效果独有参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|依附效果|周期", DisplayName="初始化周期依附效果参数")
	void InitializePeriodicAttachEffectParameters(const FLxSkillPeriodicAttachEffectSpec& InPeriodicSpec);

protected:
	/** 将触发间隔写入生命周期周期时钟。 */
	virtual void ApplySkillUnitSpecToComponents() override;

	/** 绑定周期时钟事件。 */
	virtual void BindSkillUnitComponentEvents() override;

	/** 检查触发间隔是否能够启动周期依附效果。 */
	virtual bool CanActivateAttachEffect() const override;

	/** 根据配置决定依附成功后是否立即命中一次。 */
	virtual void HandleAttachEffectActivated() override;

	/** 生命周期周期到达时为当前依附目标广播命中。 */
	UFUNCTION()
	void HandleAttachEffectPeriod(float RemainingTime);

	/** 周期依附效果独有参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|依附效果|周期", DisplayName="周期依附效果参数")
	FLxSkillPeriodicAttachEffectSpec PeriodicSpec;
};