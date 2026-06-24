#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationNode.h"
#include "LxDefaultDamageCalculationNodes.generated.h"

/** 攻击力伤害输出节点，按攻击力上下限随机生成普通伤害。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="攻击力伤害输出节点")
class LXARPG_API ULxDamageAttackPowerOutputNode : public ULxDamageCalculationNode
{
	GENERATED_BODY()

public:
	/** 创建攻击力伤害输出节点。 */
	ULxDamageAttackPowerOutputNode();

	virtual void ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext) override;

protected:
	/** 输出伤害类型标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="输出伤害类型标签", meta=(Categories="通用效果.伤害效果"))
	FGameplayTag DamageTypeTag;
};

/** 暴击输出节点，按暴击率和暴击伤害放大当前伤害。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="暴击伤害输出节点")
class LXARPG_API ULxDamageCriticalOutputNode : public ULxDamageCalculationNode
{
	GENERATED_BODY()

public:
	virtual void ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext) override;

protected:
	/** 没有配置暴击伤害属性时使用的默认暴击倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="默认暴击倍率")
	float DefaultCriticalDamageMultiplier = 2.f;
};

/** 防御力伤害接收节点，使用递减公式降低全部伤害。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="防御力伤害接收节点")
class LXARPG_API ULxDamageDefenseReceiveNode : public ULxDamageCalculationNode
{
	GENERATED_BODY()

public:
	virtual void ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext) override;
};

/** 护盾生命结算节点，将最终伤害拆分为护盾承受和生命承受。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="护盾生命结算节点")
class LXARPG_API ULxDamageShieldHealthSettlementNode : public ULxDamageCalculationNode
{
	GENERATED_BODY()

public:
	virtual void ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext) override;
};
