#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAttributeCoreType.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxTypedAttributeData.generated.h"

/** 各类角色属性共享的标识和显示信息，不包含任何具体数值字段。 */
USTRUCT(BlueprintType, DisplayName="角色属性公共信息")
struct LXARPG_API FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 属性ID标签，用于分类和索引。 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性", DisplayName="属性ID标签", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 属性分类，由 C++ 注册函数写入，蓝图配置中不可修改。 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性", DisplayName="属性分类类型")
	ELxCharacterAttributeCategoryType AttributeCategory = ELxCharacterAttributeCategoryType::Numeric;

	/** 属性在界面中使用的名称、描述和可见性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性", DisplayName="属性可视化信息")
	FLxAttributeShowInfo ShowInfo;
};

/** 基础属性数据；基础值可以通过衍生规则影响其他属性。 */
USTRUCT(BlueprintType, DisplayName="基础属性数据")
struct LXARPG_API FLxBasicAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前基础属性值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="属性值")
	float Value = 0.f;

	/** 当前基础属性对其他属性产生的衍生规则。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|基础", DisplayName="衍生规则列表")
	TArray<FLxAttributeDerivedRule> DerivedRules;
};

/** 资源属性数据，分别保存上限值和运行时有效值。 */
USTRUCT(BlueprintType, DisplayName="资源属性数据")
struct LXARPG_API FLxResourceAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前资源能够达到的上限值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="属性上限值")
	float ValueLimit = 0.f;

	/** 当前资源在运行时的有效值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|资源", DisplayName="属性有效值")
	float Value = 0.f;
};

/** 几率属性数据，数值为1时表示判定必定成功。 */
USTRUCT(BlueprintType, DisplayName="几率属性数据")
struct LXARPG_API FLxProbabilityAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前几率值，使用0到1的比例表达。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|几率", DisplayName="属性值")
	float Value = 0.f;
};

/** 百分比属性数据，数值为1时表示100%。 */
USTRUCT(BlueprintType, DisplayName="百分比属性数据")
struct LXARPG_API FLxPercentageAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前百分比属性值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|百分比", DisplayName="属性值")
	float Value = 0.f;
};

/** 数值属性数据，用于存储直接参与计算的有效数值。 */
USTRUCT(BlueprintType, DisplayName="数值属性数据")
struct LXARPG_API FLxNumericAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前数值属性的有效值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|数值", DisplayName="属性值")
	float Value = 0.f;
};

/** 区间属性数据，用基准值及两个方向的浮动比例定义区间。 */
USTRUCT(BlueprintType, DisplayName="区间属性数据")
struct LXARPG_API FLxRangeAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前区间属性的基准值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|区间", DisplayName="属性基准值")
	float Value = 0.f;

	/** 区间值向提高方向允许的最大浮动比例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|区间", DisplayName="提高区间比例")
	float UpwardFloatingRatio = 1.f;

	/** 区间值向降低方向允许的最大浮动比例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|基础属性|区间", DisplayName="降低区间比例")
	float DownwardFloatingRatio = 1.f;
};

/** 基础属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="基础属性修改数据")
struct LXARPG_API FLxBasicAttributeModifier
{
	GENERATED_BODY()
	/** 目标基础属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="目标属性ID", meta=(Categories="属性")) FGameplayTag AttributeIDTag;
	/** 基础属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改方式") ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;
	/** 基础属性修改数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改数值") float Value = 0.f;
};

/** 资源属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="资源属性修改数据")
struct LXARPG_API FLxResourceAttributeModifier
{
	GENERATED_BODY()
	/** 目标资源属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="目标属性ID", meta=(Categories="属性")) FGameplayTag AttributeIDTag;
	/** 资源属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改方式") ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;
	/** 资源上限值修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="属性上限值修改值") float ValueLimit = 0.f;
	/** 资源有效值修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="属性有效值修改值") float Value = 0.f;
};

/** 几率属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="几率属性修改数据")
struct LXARPG_API FLxProbabilityAttributeModifier
{
	GENERATED_BODY()
	/** 目标几率属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="目标属性ID", meta=(Categories="属性")) FGameplayTag AttributeIDTag;
	/** 几率属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改方式") ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;
	/** 几率属性修改数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改数值") float Value = 0.f;
};

/** 百分比属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="百分比属性修改数据")
struct LXARPG_API FLxPercentageAttributeModifier
{
	GENERATED_BODY()
	/** 目标百分比属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="目标属性ID", meta=(Categories="属性")) FGameplayTag AttributeIDTag;
	/** 百分比属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改方式") ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;
	/** 百分比属性修改数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改数值") float Value = 0.f;
};

/** 数值属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="数值属性修改数据")
struct LXARPG_API FLxNumericAttributeModifier
{
	GENERATED_BODY()
	/** 目标数值属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="目标属性ID", meta=(Categories="属性")) FGameplayTag AttributeIDTag;
	/** 数值属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改方式") ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;
	/** 数值属性修改数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改数值") float Value = 0.f;
};

/** 区间属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="区间属性修改数据")
struct LXARPG_API FLxRangeAttributeModifier
{
	GENERATED_BODY()
	/** 目标区间属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="目标属性ID", meta=(Categories="属性")) FGameplayTag AttributeIDTag;
	/** 区间属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改方式") ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;
	/** 区间基准值修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改基准值数值") float Value = 0.f;
	/** 提高区间比例修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改提高区间比例数值") float UpwardFloatingRatio = 0.f;
	/** 降低区间比例修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|基础属性|修改", DisplayName="修改降低区间比例数值") float DownwardFloatingRatio = 0.f;
};

/** 六类独立属性的网络传输快照；仅负责分组传输，不作为单个属性的存储类型。 */
USTRUCT(BlueprintType, DisplayName="角色分类属性网络快照")
struct LXARPG_API FLxTypedAttributeSnapshot
{
	GENERATED_BODY()

	/** 基础属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性|网络", DisplayName="基础属性快照") TArray<FLxBasicAttributeData> BasicAttributes;
	/** 资源属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性|网络", DisplayName="资源属性快照") TArray<FLxResourceAttributeData> ResourceAttributes;
	/** 几率属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性|网络", DisplayName="几率属性快照") TArray<FLxProbabilityAttributeData> ProbabilityAttributes;
	/** 百分比属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性|网络", DisplayName="百分比属性快照") TArray<FLxPercentageAttributeData> PercentageAttributes;
	/** 数值属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性|网络", DisplayName="数值属性快照") TArray<FLxNumericAttributeData> NumericAttributes;
	/** 区间属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|基础属性|网络", DisplayName="区间属性快照") TArray<FLxRangeAttributeData> RangeAttributes;
};
