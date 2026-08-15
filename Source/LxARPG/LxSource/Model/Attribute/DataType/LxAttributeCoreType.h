#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAttributeEnumType.h"
#include "LxAttributeCoreType.generated.h"

/** 属性可视化信息，用于描述属性在 UI 中的显示内容。 */
USTRUCT(BlueprintType, DisplayName="属性可视化信息")
struct FLxAttributeShowInfo
{
	GENERATED_BODY()

	/** 属性在 UI 中显示的名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性可视化名称")
	FText AttributeName;

	/** 属性名称使用的富文本显示样式标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性名称显示样式标签", meta=(Categories="文本样式"))
	FGameplayTag AttributeNameStyleIDTag;

	/** 属性说明文本，用于详情面板或悬浮提示。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性可视化描述")
	FText AttributeDescription;

	/** 是否在角色属性列表中显示该属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="是否在列表中显示")
	bool IsVisible = false;
};

/** 标量属性约束与显示配置，用数据描述几率、百分比和普通数值之间的逻辑差异。 */
USTRUCT(BlueprintType, DisplayName="标量属性规则")
struct FLxScalarAttributeRule
{
	GENERATED_BODY()

	/** 是否限制属性最小值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性|标量规则", DisplayName="限制最小值")
	bool bClampMinimum = false;

	/** 属性允许的最小值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性|标量规则", DisplayName="最小值", meta=(EditCondition="bClampMinimum"))
	float MinimumValue = 0.f;

	/** 是否限制属性最大值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性|标量规则", DisplayName="限制最大值")
	bool bClampMaximum = false;

	/** 属性允许的最大值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性|标量规则", DisplayName="最大值", meta=(EditCondition="bClampMaximum"))
	float MaximumValue = 1.f;

	/** 是否在运行时把属性值四舍五入为整数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性|标量规则", DisplayName="运行时取整")
	bool bRoundValue = false;

	/** 属性数值在界面中的显示格式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性|标量规则", DisplayName="显示格式")
	ELxScalarAttributeDisplayFormat DisplayFormat = ELxScalarAttributeDisplayFormat::Integer;

	/** 按当前规则完成取整与上下限约束。 */
	float NormalizeValue(float InValue) const
	{
		float Result = bRoundValue ? FMath::RoundToFloat(InValue) : InValue;
		if (bClampMinimum) Result = FMath::Max(Result, MinimumValue);
		if (bClampMaximum) Result = FMath::Min(Result, MaximumValue);
		return Result;
	}
};
