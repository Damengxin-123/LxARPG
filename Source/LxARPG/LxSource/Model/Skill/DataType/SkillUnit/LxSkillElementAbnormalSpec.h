#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxSkillElementAbnormalSpec.generated.h"

class UNiagaraSystem;

/** 元素异常持续期间维持的单个 Buff，持续时间统一由依附单元管理。 */
USTRUCT(BlueprintType, DisplayName="异常维持Buff参数")
struct FLxElementAbnormalBuffSpec
{
	GENERATED_BODY()

	/** 已在 Buff 物品表中配置的标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常", DisplayName="Buff标签", meta=(Categories="物品.buff"))
	FGameplayTag BuffIDTag;

	/** 传入现有 Buff 系统的效果比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常", DisplayName="效果比例", meta=(ClampMin="0.0"))
	float EffectProportion = 1.f;
};

/** 从前置命中目标施加元素异常的参数；本阶段不包含免疫与元素自动触发规则。 */
USTRUCT(BlueprintType, DisplayName="元素异常参数")
struct LXARPG_API FLxSkillElementAbnormalSpec
{
	GENERATED_BODY()

	/** 每个依附单元只在首次激活时判定一次；零必定失败，百分之百必定成功。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常", DisplayName="触发几率（%）", meta=(ClampMin="0.0", ClampMax="100.0"))
	float ProcChance = 100.f;

	/** 异常期间维持的角色状态标签；撤回时保留其他来源的相同标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常", DisplayName="异常状态标签", meta=(Categories="角色状态.元素异常状态"))
	FGameplayTagContainer StateTags;

	/** 异常生效后施加的 Buff；不同异常单元各自持有独立实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常", DisplayName="维持Buff列表")
	TArray<FLxElementAbnormalBuffSpec> Buffs;

	/** 单次周期的基础伤害，0 表示关闭；由技能单元提交标准伤害流程，不直接修改生命值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常|周期伤害", DisplayName="每次基础伤害", meta=(ClampMin="0.0"))
	float DamagePerTick = 0.f;

	/** 周期伤害类型，例如火焰伤害；伤害大于零时必须配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常|周期伤害", DisplayName="伤害类型", meta=(Categories="通用效果.伤害效果"))
	FGameplayTag DamageTypeTag;

	/** 首次等待完整间隔，此后按同一周期结算；计时由技能生命周期组件驱动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常|周期伤害", DisplayName="伤害间隔（秒）", meta=(ClampMin="0.1"))
	float DamageInterval = 1.f;

	/** 可选的 Niagara（粒子特效）资源，只在异常成功生效后播放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|元素异常", DisplayName="异常视觉效果")
	TSoftObjectPtr<UNiagaraSystem> VisualEffect;

	/** 校验概率、标签分类、周期伤害和 Buff 参数，不执行随机判定。 */
	bool IsValid() const;
};
