#pragma once

#include "CoreMinimal.h"
#include "LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMeleeSpec.h"
#include "LxMeleeSkillUnitActor.generated.h"

class ALxMeleeSkillUnitActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxMeleeHitTarget, ALxMeleeSkillUnitActor*, MeleeSkillUnit, const FLxMeleeHitContext&, HitContext);

/** 近战效果技能单元，接收角色近身战斗组件确认的有效武器命中并向技能输出事件。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="近战效果技能单元")
class LXARPG_API ALxMeleeSkillUnitActor : public ALxSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxMeleeSkillUnitActor();

	/** 初始化近战效果参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|近战", DisplayName="初始化近战效果参数")
	void InitializeMeleeParameters(const FLxSkillMeleeSpec& InMeleeSpec);

	/** 获取近战组件开始攻击时需要使用的命中限制配置。 */
	UFUNCTION(BlueprintPure, Category="技能单元|近战", DisplayName="获取近战效果参数")
	FLxSkillMeleeSpec GetMeleeSpec() const { return MeleeSpec; }

	/** 接收已经由近身战斗组件筛选为有效的单次命中；返回是否被本单元接受。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|近战|命中", DisplayName="接收武器命中")
	bool ReceiveWeaponHit(const FLxMeleeHitContext& InHitContext);

	/** 获取本次释放已经接受的有效武器命中次数。 */
	UFUNCTION(BlueprintPure, Category="技能单元|近战|命中", DisplayName="获取当前命中次数")
	int32 GetCurrentHitCount() const { return CurrentHitCount; }

	/** 每次接受有效武器命中时触发，只输出本次命中的单个目标和命中位置。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|近战|事件", DisplayName="命中目标事件")
	FOnLxMeleeHitTarget OnMeleeHitTarget;

protected:
	virtual void ActivateSkillUnit_Implementation() override;

	/** 近战效果单元运行参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|近战", DisplayName="近战效果参数")
	FLxSkillMeleeSpec MeleeSpec;

	/** 本次释放已经接受的有效武器命中次数。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|近战|运行状态", DisplayName="当前命中次数")
	int32 CurrentHitCount = 0;
};
