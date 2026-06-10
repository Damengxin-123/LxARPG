// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryEnum.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxAttributeCoreType.generated.h"

/**
 * 属性可视化信息。
 *
 * 用于描述属性在 UI 中展示时需要的名称、说明文本和显示开关。
 */
USTRUCT(BlueprintType, DisplayName="属性可视化信息")
struct FLxAttributeShowInfo
{
	GENERATED_BODY()

	/** 属性在 UI 中显示的名称，可由富文本片段组合而成。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="属性可视化名称")
	FText AttributeName;

	/** 属性的说明文本，用于详情面板、悬浮提示等展示。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="属性可视化描述")
	FText AttributeDescription;

	/** 是否在角色属性列表中显示该属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="是否在列表中显示")
	bool IsVisible = false;
};

/**
 * 角色属性数值信息。
 *
 * 保存一个属性的当前值、上限、浮动范围和数值类型。
 */
USTRUCT(BlueprintType, DisplayName="角色属性值信息")
struct FLxAttributeValue
{
	GENERATED_BODY()

	/** 属性允许达到的最大值；对无上限属性可保持默认值或由业务逻辑忽略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="数值上限")
	float ValueLimit = 0;

	/** 属性当前生效值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="当前有效值")
	float Value = 0;

	/** 属性向上随机浮动或成长浮动的比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="向上浮动比例")
	float UpwardFloatingRatio = 1;

	/** 属性向下随机浮动或成长浮动的比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="向下浮动比例")
	float DownwardFloatingRatio = 1;

	/** 属性值的解释方式，例如固定值、区间值、百分比等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="角色属性值类型")
	ELxCharacterValueType ValueType = ELxCharacterValueType::FixedNumeric;

	FLxAttributeValue() {}
};

/**
 * 属性衍生规则。
 *
 * 描述当前属性如何按一定比例影响另一个属性，例如力量衍生生命上限。
 */
USTRUCT(BlueprintType, DisplayName="属性衍生规则")
struct FLxAttributeDerivedRule
{
	GENERATED_BODY()

	/** 被当前属性加成或衍生影响的目标属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="加成对象")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	/** 被当前属性加成或衍生影响的目标属性标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性", DisplayName="加成对象标签", meta=(Categories="Attribute"))
	FGameplayTag AttributeIDTag;

	/** 衍生结果要作用到目标属性值结构中的哪个字段。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="作用目标")
	ELxEntryTarget EntryTarget = ELxEntryTarget::ToValue;

	/** 加成方式，决定按基础值、最终值或其他规则计算。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="加成方式")
	ELxEntryEffectiveType EffectiveType = ELxEntryEffectiveType::BasicValue;

	/** 转化比例；最终衍生值由来源属性值和该比例共同决定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute", DisplayName="转化比例")
	float Ratio = 1.f;

	FLxAttributeDerivedRule() {}

	FLxAttributeDerivedRule(ELxCharacterAttributeID AttributeID,
		ELxEntryEffectiveType EffectiveType, float Ratio)
	{
		this->AttributeID = AttributeID;
		this->EffectiveType = EffectiveType;
		this->Ratio = Ratio;
	}
};
