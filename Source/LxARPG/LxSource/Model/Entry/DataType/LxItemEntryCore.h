// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxItemEntryEnum.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffEnum.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"
#include "LxItemEntryCore.generated.h"

/**
 * 词条数值信息。
 * 用于描述词条最终作用到目标时所需的数值、数值类型、作用目标，以及周期性生效间隔。
 */
USTRUCT(BlueprintType, DisplayName="词条数值信息")
struct FLxEntryValueInfo
{
	GENERATED_BODY()

	/** 词条基础数值。实际使用时通常还会乘以词条生效比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="词条数值")
	float Value = 0.f;

	/** 数值周期性生效的间隔，单位为秒；值为 -1 时表示普通加成词条，不参与时间折算。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="生效周期-秒")
	float EffectivePeriod = -1.f;

	/** 词条数值的加成方式，例如基础数值、基础提高、额外提高或机制型效果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="加成方式")
	ELxItemEntryType EntryType = ELxItemEntryType::BasicValue;

	/** 词条数值的作用目标，例如作用到属性上限或当前值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="加成目标")
	ELxItemEntryTarget EntryTarget = ELxItemEntryTarget::ToValue;
};

/**
 * 词条基础信息。
 * 用于标识词条 ID、词条逻辑类型，以及词条自身和作用目标的 GameplayTag。
 */
USTRUCT(BlueprintType, DisplayName="词条基础信息")
struct FLxEnteryBaseInfo
{
	GENERATED_BODY()

	/** 词条唯一 ID，对应词条数据表中的行数据标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="词条ID")
	FName EntryID;

	/** 词条逻辑类型，用于决定词条组件如何分类和派发该词条。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="词条逻辑类型")
	ELxEntryLogicType EntryLogicType = ELxEntryLogicType::ChangeAttributeValue;

	/** 词条自身标签，用于描述词条所属模块、功能或分类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条|标签", DisplayName="词条标签", meta=(Categories="Module,Attribute,Function,Trait,SkillForm"))
	FGameplayTagContainer EntryTags;

	/** 词条作用目标标签，用于描述该词条影响的属性、功能或目标对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条|标签", DisplayName="目标标签", meta=(Categories="Module,Attribute,Function,Trait,SkillForm"))
	FGameplayTagContainer TargetTags;

	FLxEnteryBaseInfo()
	{
		EntryTags.AddTag(LxTag_Module_Entry);
	}
};

/**
 * 词条显示信息。
 * 用于配置词条名称和描述文本，供 UI 拼接并展示词条说明。
 */
USTRUCT(BlueprintType, DisplayName="词条显示信息")
struct FLxEnteryShowInfo
{
	GENERATED_BODY()

	/** 词条名称富文本数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="显示名称")
	FLxRichTextDescriptionGroupData DisplayNameData;

	/** 词条描述文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="描述文本")
	FText Description;
};

/**
 * 修改角色属性词条数据。
 * 用于描述直接修改角色属性的词条数值信息。
 */
USTRUCT(BlueprintType, DisplayName="修改属性词条数据")
struct FLxChangeAttributeValue
{
	GENERATED_BODY()

	/** 修改属性时使用的数值信息。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="词条数值信息")
	FLxEntryValueInfo EntryValueInfo;
};

/**
 * 修改角色状态词条数据。
 * 当前作为状态类词条的扩展结构预留。
 */
USTRUCT(BlueprintType, DisplayName="修改状态词条数据")
struct FLxChangeStateValue
{
	GENERATED_BODY()
};

/**
 * 创建 Buff 词条数据。
 * 用于描述使用物品或触发词条时需要创建的 Buff ID、持续时间和效果比例。
 */
USTRUCT(BlueprintType, DisplayName="创建Buff词条数据")
struct FLxCreaterBufferValue
{
	GENERATED_BODY()

	/** 创建出的 Buff 持续时间，单位为秒；负数表示不主动失效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条|Buff", DisplayName="Buff持续时间-秒")
	float DurationOnS = 0.f;

	/** 需要创建的 Buff 类型 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条|Buff", DisplayName="BuffID")
	ELxBuffID BuffID = ELxBuffID::None;

	/** 创建 Buff 时传入的效果比例，用于影响 Buff 中词条的最终数值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条|Buff", DisplayName="效果比例")
	float ValueProportion = 0.f;
};

/**
 * Buff 生效词条数据。
 * 用于描述 Buff 周期触发时实际作用到角色属性或状态上的词条数值。
 */
USTRUCT(BlueprintType, DisplayName="Buff生效词条数据")
struct FLxBufferEnterValue
{
	GENERATED_BODY()

	/** Buff 生效时使用的数值信息。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="词条数值信息")
	FLxEntryValueInfo EntryValueInfo;
};

/**
 * 文本词条数据。
 * 当前用于描述类或提示类词条的扩展结构预留。
 */
USTRUCT(BlueprintType, DisplayName="文本词条数据")
struct FLxTextEnterValue
{
	GENERATED_BODY()
};
