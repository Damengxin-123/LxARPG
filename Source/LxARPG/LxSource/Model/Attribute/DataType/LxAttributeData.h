#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LxAttributeCoreType.h"
#include "LxAttributeData.generated.h"

/**
 * 角色属性信息配置。
 *
 * 数据表行结构体，用于描述一项属性的基础值、标签、衍生规则和 UI 展示信息。
 */
USTRUCT(BlueprintType, DisplayName="角色属性信息配置")
struct FLxAttributeData : public FTableRowBase
{
	GENERATED_BODY()

	/** 角色属性标签 ID，用于运行时查找和配置属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性ID标签", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 属性关联的目标标签，用于词条过滤和条件匹配。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性标签")
	FGameplayTagContainer TargetTags;

	/** 属性基础值，不包含装备、Buff 或其他运行时加成。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="基础属性值")
	FLxAttributeValue AttributeValue;

	/** 属性对其他属性的衍生或加成规则列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性衍生规则")
	TArray<FLxAttributeDerivedRule> DerivedRulesArray;

	/** 属性在 UI 中展示时使用的名称、描述和可见性配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性可视化信息")
	FLxAttributeShowInfo ShowInfo;

	/** 运行时计算后的最终属性值，由属性组件维护，不应在数据表中手动配置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色属性", DisplayName="运行时计算属性值")
	FLxAttributeValue CalculatedAttributeValue;

	/** 创建角色属性信息配置。 */
	FLxAttributeData() {}
};

/**
 * 种族基础属性值配置。
 *
 * 数据表行结构体，用于按种族或角色模板覆盖某一项属性的基础数值。
 */
USTRUCT(BlueprintType, DisplayName="种族基础属性值配置")
struct FLxAttributeValueConfig : public FTableRowBase
{
	GENERATED_BODY()

	/** 角色属性标签 ID，用于按标签覆盖指定属性的基础数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="属性ID标签", meta=(Categories="属性"))
	FGameplayTag AttributeIDTag;

	/** 该属性的基础数值上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="数值上限")
	float ValueLimit = 0.f;

	/** 该属性的基础当前值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="当前值")
	float Value = 0.f;

	/** 基础值向上浮动比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="向上浮动比例")
	float UpwardFloatingRatio = 1.f;

	/** 基础值向下浮动比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="向下浮动比例")
	float DownwardFloatingRatio = 1.f;
};
