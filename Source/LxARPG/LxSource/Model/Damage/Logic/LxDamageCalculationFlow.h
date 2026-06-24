#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxDamageCalculationFlow.generated.h"

/** 伤害计算完成事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxDamageCalculationFinished, const FLxDamageCalculationContext&, DamageCalculationContext);

/** 伤害计算流程配置类型，蓝图子类可以重写流程 API 并手动串联各级计算函数。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="伤害计算流程")
class LXARPG_API ULxDamageCalculationFlow : public UObject
{
	GENERATED_BODY()

public:
	/** 执行伤害输出流程 API，蓝图子类可重写后自行串联攻击力、暴击等计算函数。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="伤害计算|流程API", DisplayName="计算伤害输出流程")
	FLxDamageCalculationContext CalculateOutgoingDamage(const FLxDamageCalculationContext& InDamageContext);
	virtual FLxDamageCalculationContext CalculateOutgoingDamage_Implementation(const FLxDamageCalculationContext& InDamageContext);

	/** 执行伤害接收流程 API，蓝图子类可重写后自行串联防御、资源结算等计算函数。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="伤害计算|流程API", DisplayName="计算伤害接收流程")
	FLxDamageCalculationContext CalculateIncomingDamage(const FLxDamageCalculationContext& InDamageContext);
	virtual FLxDamageCalculationContext CalculateIncomingDamage_Implementation(const FLxDamageCalculationContext& InDamageContext);

	/** 按技能伤害配置读取来源角色属性，并按倍率生成对应类型伤害。 */
	UFUNCTION(BlueprintCallable, Category="伤害计算|输出节点", DisplayName="节点-按来源属性生成技能伤害")
	FLxDamageCalculationContext GenerateDamageFromSourceAttributes(const FLxDamageCalculationContext& InDamageContext);

	/** 按攻击力上下限随机生成普通伤害。 */
	UFUNCTION(BlueprintCallable, Category="伤害计算|输出节点", DisplayName="节点-攻击力生成普通伤害")
	FLxDamageCalculationContext CalculateAttackPowerDamageOutput(const FLxDamageCalculationContext& InDamageContext);

	/** 按暴击率和暴击伤害放大当前伤害。 */
	UFUNCTION(BlueprintCallable, Category="伤害计算|输出节点", DisplayName="节点-计算暴击伤害")
	FLxDamageCalculationContext CalculateCriticalDamageOutput(const FLxDamageCalculationContext& InDamageContext);

	/** 按目标防御力递减公式降低当前伤害。 */
	UFUNCTION(BlueprintCallable, Category="伤害计算|接收节点", DisplayName="节点-计算防御减伤")
	FLxDamageCalculationContext CalculateDefenseDamageReduction(const FLxDamageCalculationContext& InDamageContext);

	/** 将最终伤害拆分为护盾和生命承受，并按需通过目标数据中转组件直接扣除资源。 */
	UFUNCTION(BlueprintCallable, Category="伤害计算|接收节点", DisplayName="节点-结算护盾生命伤害")
	FLxDamageCalculationContext ApplyShieldHealthDamageSettlement(const FLxDamageCalculationContext& InDamageContext);

	/** 伤害计算完成事件。 */
	UPROPERTY(BlueprintAssignable, Category="伤害计算", DisplayName="伤害计算完成事件")
	FOnLxDamageCalculationFinished OnDamageCalculationFinished;

protected:
	/** 没有配置暴击伤害属性时使用的默认暴击倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算|默认参数", DisplayName="默认暴击倍率")
	float DefaultCriticalDamageMultiplier = 2.f;
};
