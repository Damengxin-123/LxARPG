#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAttributeCoreType.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxTypedAttributeData.generated.h"

/** 各类角色属性共享的标识、业务分类和显示信息。 */
USTRUCT(BlueprintType, DisplayName="角色属性公共信息")
struct LXARPG_API FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 属性ID标签，用于唯一索引一个角色属性。 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="属性ID标签", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 属性在玩法中的业务分类。 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="属性业务分类")
	ELxCharacterAttributeBusinessCategory BusinessCategory = ELxCharacterAttributeBusinessCategory::Combat;

	/** 属性使用的底层数值结构。 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="属性数值结构类型")
	ELxCharacterAttributeValueType ValueType = ELxCharacterAttributeValueType::Scalar;

	/** 当前属性每1点或1%能够换算得到的角色强度数值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="角色强度换算指数")
	int32 StrengthConversionIndex = 0;

	/** 属性在界面中使用的名称、描述和可见性。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性", DisplayName="属性可视化信息")
	FLxAttributeShowInfo ShowInfo;
};

/** 标量属性数据；能力值、判定、百分比和普通数值统一使用该结构。 */
USTRUCT(BlueprintType, DisplayName="标量属性数据")
struct LXARPG_API FLxScalarAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前标量属性值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|标量", DisplayName="属性值")
	float Value = 0.f;

	/** 标量属性的范围、取整和显示规则。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|标量", DisplayName="标量属性规则")
	FLxScalarAttributeRule ScalarRule;

};

/** 资源属性数据，分别保存运行时当前值和上限值。 */
USTRUCT(BlueprintType, DisplayName="资源属性数据")
struct LXARPG_API FLxResourceAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前资源能够达到的上限值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|资源", DisplayName="属性上限值")
	float ValueLimit = 0.f;

	/** 当前资源在运行时的有效值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|资源", DisplayName="属性有效值")
	float Value = 0.f;
};

/** 区间属性数据，用基准值及两个方向的浮动比例定义数值区间。 */
USTRUCT(BlueprintType, DisplayName="区间属性数据")
struct LXARPG_API FLxRangeAttributeData : public FLxCharacterAttributeCommonData
{
	GENERATED_BODY()

	/** 当前区间属性的基准值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|区间", DisplayName="属性基准值")
	float Value = 0.f;

	/** 区间向提高方向允许的最大浮动比例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|区间", DisplayName="提高区间比例")
	float UpwardFloatingRatio = 1.f;

	/** 区间向降低方向允许的最大浮动比例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|属性|区间", DisplayName="降低区间比例")
	float DownwardFloatingRatio = 1.f;
};

/** 标量属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="标量属性修改数据")
struct LXARPG_API FLxScalarAttributeModifier
{
	GENERATED_BODY()

	/** 目标标量属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="目标属性ID", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 标量属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改方式")
	ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;

	/** 标量属性修改数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改数值")
	float Value = 0.f;
};

/** 资源属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="资源属性修改数据")
struct LXARPG_API FLxResourceAttributeModifier
{
	GENERATED_BODY()

	/** 目标资源属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="目标属性ID", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 资源属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改方式")
	ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;

	/** 资源上限值修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="属性上限值修改值")
	float ValueLimit = 0.f;

	/** 资源有效值修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="属性有效值修改值")
	float Value = 0.f;
};

/** 区间属性修改数据。 */
USTRUCT(BlueprintType, DisplayName="区间属性修改数据")
struct LXARPG_API FLxRangeAttributeModifier
{
	GENERATED_BODY()

	/** 目标区间属性ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="目标属性ID", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 区间属性修改方式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改方式")
	ELxAttributeModifierOperation Operation = ELxAttributeModifierOperation::AddValue;

	/** 区间基准值修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改基准值数值")
	float Value = 0.f;

	/** 提高区间比例修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改提高区间比例数值")
	float UpwardFloatingRatio = 0.f;

	/** 降低区间比例修改量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色|属性|修改", DisplayName="修改降低区间比例数值")
	float DownwardFloatingRatio = 0.f;
};

/** 三种数值结构的角色属性网络快照。 */
USTRUCT(BlueprintType, DisplayName="角色属性网络快照")
struct LXARPG_API FLxTypedAttributeSnapshot
{
	GENERATED_BODY()

	/** 标量属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|网络", DisplayName="标量属性快照")
	TArray<FLxScalarAttributeData> ScalarAttributes;

	/** 资源属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|网络", DisplayName="资源属性快照")
	TArray<FLxResourceAttributeData> ResourceAttributes;

	/** 区间属性快照。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|网络", DisplayName="区间属性快照")
	TArray<FLxRangeAttributeData> RangeAttributes;
};

/** 只用于 UI 展示的角色属性数据。 */
USTRUCT(BlueprintType, DisplayName="角色属性显示数据")
struct LXARPG_API FLxAttributeDisplayData
{
	GENERATED_BODY()

	/** 显示数据对应的属性ID。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="属性ID标签")
	FGameplayTag AttributeIDTag;

	/** 显示数据对应的业务分类。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="属性业务分类")
	ELxCharacterAttributeBusinessCategory BusinessCategory = ELxCharacterAttributeBusinessCategory::Combat;

	/** 属性名称、描述、样式及可见性。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="属性可视化信息")
	FLxAttributeShowInfo ShowInfo;

	/** 已按属性规则格式化的数值文本。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="属性数值文本")
	FText ValueText;

	/** 套用名称样式并填充数值后的完整富文本。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="属性完整显示文本")
	FText DisplayText;

	/** 可供进度条或其他数值控件使用的当前值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="当前值")
	float Value = 0.f;

	/** 资源属性的上限值，其他数值结构保持为零。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="上限值")
	float ValueLimit = 0.f;

	/** 当前显示数据是否具有有效的上限值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色|属性|显示", DisplayName="是否具有上限值")
	bool bHasValueLimit = false;
};
