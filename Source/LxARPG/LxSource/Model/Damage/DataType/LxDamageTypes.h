#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxDamageTypes.generated.h"

/** 单条伤害数值，用伤害类型标签区分普通、火焰、破甲等伤害来源。 */
USTRUCT(BlueprintType, DisplayName="伤害数值")
struct LXARPG_API FLxDamageValue
{
	GENERATED_BODY()

	/** 伤害类型标签，例如普通伤害、火焰伤害或破甲伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害", DisplayName="伤害类型标签", meta=(Categories="通用效果.伤害效果"))
	FGameplayTag DamageTypeTag;

	/** 生成此条伤害时读取的来源角色属性标签；为空时直接使用伤害数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害", DisplayName="来源属性标签", meta=(Categories="属性"))
	FGameplayTag SourceAttributeIDTag;

	/** 来源属性倍率，1.5 表示使用来源属性的 150%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害", DisplayName="来源属性倍率", meta=(ClampMin="0.0", UIMin="0.0"))
	float SourceAttributeRatio = 1.f;

	/** 伤害数值；如果配置了来源属性标签，该值会在伤害流程中由来源属性和倍率生成。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="伤害", DisplayName="伤害数值")
	float DamageValue = 0.f;

	/** 判断该伤害数值是否可以参与计算。 */
	bool IsValidDamage() const
	{
		return DamageValue > 0.f;
	}
};
