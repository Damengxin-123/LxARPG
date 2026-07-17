#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageTypes.h"
#include "LxEffectTypes.generated.h"

class AActor;
class UObject;

/** 效果包来源类型，用于标记一组跨模块效果由哪个业务模块生成。 */
UENUM(BlueprintType, DisplayName = "效果包来源类型")
enum class ELxEffectPackageSource : uint8
{
	None UMETA(DisplayName = "无"),
	Backpack UMETA(DisplayName = "背包"),
	Equipment UMETA(DisplayName = "装备"),
	Buff UMETA(DisplayName = "Buff"),
	Skill UMETA(DisplayName = "技能"),
	Profession UMETA(DisplayName = "职业"),
	Interaction UMETA(DisplayName = "交互"),
	Other UMETA(DisplayName = "其他")
};

/** 效果包应用策略，用于决定同来源旧效果包是否被替换。 */
UENUM(BlueprintType, DisplayName = "效果包应用策略")
enum class ELxEffectPackageApplyPolicy : uint8
{
	Append UMETA(DisplayName = "追加"),
	ReplaceSameSource UMETA(DisplayName = "替换同来源"),
	Instant UMETA(DisplayName = "即时生效")
};

/** 属性修改目标，表示效果作用到属性数值结构中的哪个字段。 */
UENUM(BlueprintType, DisplayName = "属性修改目标")
enum class ELxAttributeModifierTarget : uint8
{
	ToValue UMETA(DisplayName = "当前有效值"),
	ToValueLimit UMETA(DisplayName = "数值上限"),
	ToUpwardFloatingRatio UMETA(DisplayName = "向上浮动比例"),
	ToDownwardFloatingRatio UMETA(DisplayName = "向下浮动比例")
};

/** 属性修改方式，用于表达加值、提高和总增等属性增益减益语义。 */
UENUM(BlueprintType, DisplayName = "属性修改方式")
enum class ELxAttributeModifierOperation : uint8
{
	AddValue UMETA(DisplayName = "加属性值"),
	AddBasePercent UMETA(DisplayName = "提高属性值"),
	AddTotalPercent UMETA(DisplayName = "总增属性值"),
	UseMaximumValue UMETA(DisplayName = "取较大值"),
	UseMinimumValue UMETA(DisplayName = "取较小值")
};

/** 状态修改方式，用于描述状态标签的添加、移除和切换。 */
UENUM(BlueprintType, DisplayName = "状态修改方式")
enum class ELxStateEffectOperation : uint8
{
	Add UMETA(DisplayName = "添加"),
	Remove UMETA(DisplayName = "移除"),
	Toggle UMETA(DisplayName = "切换")
};

/** 效果来源上下文，用于跨模块和跨角色传递时标记效果包来源。 */
USTRUCT(BlueprintType, DisplayName = "效果来源上下文")
struct LXARPG_API FLxEffectSourceContext
{
	GENERATED_BODY()

	/** 生成效果包的业务模块来源。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|来源", DisplayName = "来源类型")
	ELxEffectPackageSource SourceType = ELxEffectPackageSource::None;

	/** 来源角色或来源单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|来源", DisplayName = "来源角色")
	TObjectPtr<AActor> SourceActor = nullptr;

	/** 来源对象，例如装备、Buff、技能单元或交互对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|来源", DisplayName = "来源对象")
	TObjectPtr<UObject> SourceObject = nullptr;

	/** 来源标签 ID，用于同来源替换旧效果包。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|来源", DisplayName = "来源标签ID", meta = (Categories = "EffectSource"))
	FGameplayTag SourceIDTag;

	/** 来源名称，用于没有稳定标签时作为同来源替换键。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|来源", DisplayName = "来源名称")
	FName SourceName = NAME_None;

	/** 生成用于缓存和替换的来源键。 */
	FName MakeSourceKey() const;
};

/** 属性增益减益效果，作为角色属性变化的跨模块传递数据。 */
USTRUCT(BlueprintType, DisplayName = "属性增益减益效果")
struct LXARPG_API FLxAttributeModifierEffect
{
	GENERATED_BODY()

	/** 目标属性标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性", DisplayName = "目标属性标签ID", meta = (Categories = "属性"))
	FGameplayTag AttributeIDTag;

	/** 属性修改目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性", DisplayName = "属性修改目标")
	ELxAttributeModifierTarget ModifierTarget = ELxAttributeModifierTarget::ToValue;

	/** 属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性", DisplayName = "属性修改方式")
	ELxAttributeModifierOperation ModifierOperation = ELxAttributeModifierOperation::AddValue;

	/** 属性修改数值，可为负数以表达减益。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性", DisplayName = "属性修改数值")
	float ModifierValue = 0.f;

	/** 未指定属性ID时需要匹配的属性分类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="效果|属性", DisplayName="目标属性分类")
	TArray<ELxCharacterAttributeCategoryType> TargetAttributeCategories;

	/** 判断两个属性效果是否可以汇总到同一条效果。 */
	bool HasSameAggregationKey(const FLxAttributeModifierEffect& Other) const;
};

/** 属性恢复效果，表示对范围属性当前值的一次性增加。 */
USTRUCT(BlueprintType, DisplayName = "属性恢复效果")
struct LXARPG_API FLxAttributeRecoveryEffect
{
	GENERATED_BODY()

	/** 目标属性标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性恢复", DisplayName = "目标属性标签ID", meta = (Categories = "属性"))
	FGameplayTag AttributeIDTag;

	/** 恢复方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性恢复", DisplayName = "恢复方式")
	ELxAttributeModifierOperation RecoveryOperation = ELxAttributeModifierOperation::AddValue;

	/** 恢复数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|属性恢复", DisplayName = "恢复数值")
	float RecoveryValue = 0.f;

	/** 未指定属性ID时需要匹配的属性分类，恢复效果通常只选择资源属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="效果|属性恢复", DisplayName="目标属性分类")
	TArray<ELxCharacterAttributeCategoryType> TargetAttributeCategories;
};

/** 伤害效果预留数据，后续由伤害核算组件解释。 */
USTRUCT(BlueprintType, DisplayName = "伤害效果")
struct LXARPG_API FLxDamageEffect
{
	GENERATED_BODY()

	/** 伤害目标属性标签 ID，通常为生命值当前值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|伤害", DisplayName = "目标属性标签ID", meta = (Categories = "属性"))
	FGameplayTag TargetAttributeIDTag;

	/** 伤害数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|伤害", DisplayName = "伤害数值")
	float DamageValue = 0.f;

	/** 伤害元素或其他分类标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|伤害", DisplayName = "伤害标签")
	FGameplayTagContainer DamageTags;

	/** 本条伤害是否已经触发暴击。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|伤害", DisplayName = "是否暴击")
	bool bCriticalHit = false;

	/** 拆分后的伤害数值列表，可同时表达普通、火焰、破甲等多种伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|伤害", DisplayName = "伤害数值列表")
	TArray<FLxDamageValue> DamageValues;
};

/** 状态修改效果，用于向状态组件传递标签变化。 */
USTRUCT(BlueprintType, DisplayName = "状态修改效果")
struct LXARPG_API FLxStateChangeEffect
{
	GENERATED_BODY()

	/** 状态分类标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|状态", DisplayName = "状态分类标签", meta = (Categories = "CharacterState"))
	FGameplayTag StateCategoryTag;

	/** 需要修改的状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|状态", DisplayName = "状态标签", meta = (Categories = "CharacterState"))
	FGameplayTag StateTag;

	/** 状态修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|状态", DisplayName = "状态修改方式")
	ELxStateEffectOperation Operation = ELxStateEffectOperation::Add;
};

/** 创建 Buff 效果，用于让 Buff 组件创建或刷新 Buff。 */
USTRUCT(BlueprintType, DisplayName = "创建Buff效果")
struct LXARPG_API FLxBuffGrantEffect
{
	GENERATED_BODY()

	/** Buff 标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|Buff", DisplayName = "Buff标签ID", meta = (Categories = "物品"))
	FGameplayTag BuffIDTag;

	/** Buff 效果比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|Buff", DisplayName = "效果比例")
	float EffectProportion = 1.f;

	/** 持续时间，小于 0 表示永久。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|Buff", DisplayName = "持续时间")
	float Duration = -1.f;
};

/** 授予技能效果，用于让技能背包获得指定技能物品。 */
USTRUCT(BlueprintType, DisplayName = "授予技能效果")
struct LXARPG_API FLxSkillGrantEffect
{
	GENERATED_BODY()

	/** 技能物品标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果|技能", DisplayName = "技能物品标签ID", meta = (Categories = "物品.技能"))
	FGameplayTag SkillItemIDTag;
};

/** 模块效果总包，承载一个业务模块本次输出的全部分类效果。 */
USTRUCT(BlueprintType, DisplayName = "模块效果数据包")
struct LXARPG_API FLxEffectPackage
{
	GENERATED_BODY()

	/** 效果来源上下文。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包", DisplayName = "效果来源")
	FLxEffectSourceContext SourceContext;

	/** 效果目标角色或目标单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包", DisplayName = "目标角色")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 效果包应用策略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包", DisplayName = "应用策略")
	ELxEffectPackageApplyPolicy ApplyPolicy = ELxEffectPackageApplyPolicy::ReplaceSameSource;

	/** 属性增益减益效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包|属性", DisplayName = "属性增益减益效果列表")
	TArray<FLxAttributeModifierEffect> AttributeModifierEffects;

	/** 属性恢复效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包|属性", DisplayName = "属性恢复效果列表")
	TArray<FLxAttributeRecoveryEffect> AttributeRecoveryEffects;

	/** 伤害效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包|伤害", DisplayName = "伤害效果列表")
	TArray<FLxDamageEffect> DamageEffects;

	/** 状态修改效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包|状态", DisplayName = "状态修改效果列表")
	TArray<FLxStateChangeEffect> StateChangeEffects;

	/** 创建 Buff 效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包|Buff", DisplayName = "创建Buff效果列表")
	TArray<FLxBuffGrantEffect> BuffGrantEffects;

	/** 授予技能效果列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "效果包|技能", DisplayName = "授予技能效果列表")
	TArray<FLxSkillGrantEffect> SkillGrantEffects;

	/** 判断效果包是否没有任何子效果。 */
	bool IsEmpty() const;
};
