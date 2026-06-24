#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageTypes.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxDamageCalculationTypes.generated.h"

class ULxCharacterDataTransferComponent;

/** 伤害计算上下文，贯穿一次输出或接收伤害的完整流程。 */
USTRUCT(BlueprintType, DisplayName="伤害计算上下文")
struct LXARPG_API FLxDamageCalculationContext
{
	GENERATED_BODY()

	/** 发起伤害的角色或单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="伤害来源角色")
	TObjectPtr<AActor> SourceActor = nullptr;

	/** 接收伤害的角色或单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="伤害目标角色")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 来源角色数据中转组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="来源数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> SourceDataTransferComponent = nullptr;

	/** 目标角色数据中转组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="目标数据中转组件")
	TObjectPtr<ULxCharacterDataTransferComponent> TargetDataTransferComponent = nullptr;

	/** 输入效果包。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="输入效果包")
	FLxEffectPackage InputEffectPackage;

	/** 输出效果包。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="输出效果包")
	FLxEffectPackage OutputEffectPackage;

	/** 当前流程中正在计算的伤害效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="伤害效果列表")
	TArray<FLxDamageEffect> DamageEffects;

	/** 本次伤害是否触发暴击。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="是否暴击")
	bool bCriticalHit = false;

	/** 本次伤害是否被无敌等状态忽略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="是否忽略伤害")
	bool bIgnoredDamage = false;

	/** 是否在接收流程结算时直接通过目标数据中转组件应用资源扣除。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="是否应用伤害结果")
	bool bApplyResultToTarget = true;

	/** 本次由护盾承受的伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="护盾承受伤害")
	float ShieldDamageValue = 0.f;

	/** 本次由生命承受的伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="生命承受伤害")
	float HealthDamageValue = 0.f;

	/** 本次读取到的目标防御力。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害计算", DisplayName="目标防御力")
	float DefenseValue = 0.f;
};
