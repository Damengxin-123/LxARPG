#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxDamageCalculationNode.generated.h"

/** 伤害计算节点基类，每个节点负责流程中的一段独立计算。 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="伤害计算节点")
class LXARPG_API ULxDamageCalculationNode : public UObject
{
	GENERATED_BODY()

public:
	/** 执行伤害计算节点。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="伤害计算", DisplayName="执行伤害计算节点")
	void ExecuteDamageCalculation(UPARAM(ref) FLxDamageCalculationContext& InOutContext);
	virtual void ExecuteDamageCalculation_Implementation(FLxDamageCalculationContext& InOutContext);
};
