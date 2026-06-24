// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LxEntryEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ShowInfoConfig/LxItemShowInfoConfigID.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleTypes.h"
#include "LxItemEntryData.generated.h"

/**
 * 词条可视化文本。
 *
 * 用于在 UI 中展示词条名称和描述。
 */
USTRUCT(BlueprintType, DisplayName = "词条可视化文本")
struct FLxEntryText
{
	GENERATED_BODY()

	/** 词条显示名称，生成 UI 文本前会解析为富文本样式标签。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条显示名称")
	FLxRichStyledText EntryDisplayName;

	/** 词条描述文本。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条描述文本")
	FText EntryDescribeText;
};

/**
 * 词条基础数据。
 *
 * 所有具体词条数据表结构体的基类。
 */
USTRUCT(BlueprintType, DisplayName = "词条基础数据")
struct FLxEntryBase : public FTableRowBase
{
	GENERATED_BODY()

	/** 词条类型，用于决定运行时词条对象类型。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条类型")
	ELxEntryType EntryType = ELxEntryType::NoneEntryType;

	/** 词条标签 ID，作为词条数据的唯一主键。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条标签ID", meta = (Categories = "词条"))
	FGameplayTag EntryID;

	/** 词条可视化信息。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条可视化信息")
	FLxEntryText EntryText;
};

/** 属性增益词条。 */
USTRUCT(BlueprintType, DisplayName = "属性增益词条")
struct FLxEntryAttributeGain : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryAttributeGain()
	{
		EntryType = ELxEntryType::AttributeGain;
	}

	/** 作用属性标签 ID，用于按 GameplayTag 指定词条影响的属性。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "作用属性标签ID", meta = (Categories = "属性"))
	FGameplayTag AttributeIDTag;

	/** 作用对象。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "作用对象")
	ELxEntryTarget EntryTarget = ELxEntryTarget::ToValue;

	/** 作用方式。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "作用方式")
	ELxEntryEffectiveType EffectiveType = ELxEntryEffectiveType::BasicValue;

	/** 词条数值。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条数值")
	float EntryValue = 0.f;

	/** 目标标签。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "目标标签")
	FGameplayTagContainer TargetTags;
};

/** 属性恢复词条。 */
USTRUCT(BlueprintType, DisplayName = "属性恢复词条")
struct FLxEntryAttributeRecovery : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryAttributeRecovery()
	{
		EntryType = ELxEntryType::AttributeRecovery;
	}

	/** 作用属性标签 ID，用于按 GameplayTag 指定词条恢复的属性。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "作用属性标签ID", meta = (Categories = "属性"))
	FGameplayTag AttributeIDTag;

	/** 作用方式。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "作用方式")
	ELxEntryEffectiveType EffectiveType = ELxEntryEffectiveType::BasicValue;

	/** 恢复数值；有持续时间时表示每秒恢复值。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "恢复数值")
	float EntryValue = 0.f;

	/** 目标标签。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "目标标签")
	FGameplayTagContainer TargetTags;
};

/** 状态改变词条。 */
USTRUCT(BlueprintType, DisplayName = "状态改变词条")
struct FLxEntryChangeState : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryChangeState()
	{
		EntryType = ELxEntryType::ChangeState;
	}

	/** 状态 ID。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "状态ID")
	uint8 StateID = 0;

	/** 状态分类标签，用于新效果包状态系统识别状态所属分类。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "状态分类标签", meta = (Categories = "角色状态"))
	FGameplayTag StateCategoryTag;

	/** 状态标签，用于新效果包状态系统识别需要修改的状态。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "状态标签", meta = (Categories = "角色状态"))
	FGameplayTag StateTag;

	/** 状态词条值。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "状态词条值")
	ELxEntryStateValue StateValue = ELxEntryStateValue::Add;
};

/** 创建 Buff 词条。 */
USTRUCT(BlueprintType, DisplayName = "创建Buff词条")
struct FLxEntryCreateBuff : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryCreateBuff()
	{
		EntryType = ELxEntryType::CreateBuff;
	}

	/** Buff 标签 ID。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "Buff标签ID", meta = (Categories = "物品"))
	FGameplayTag BuffIDTag;

	/** 持续时间。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "持续时间")
	float BuffDuration = -1.f;
};

/** 多目标词条。 */
USTRUCT(BlueprintType, DisplayName = "多目标词条")
struct FLxEntryMultiTarget : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryMultiTarget()
	{
		EntryType = ELxEntryType::MultiTarget;
	}

	/** 目标标签。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "目标标签")
	FGameplayTagContainer TargetTags;
};

/** 显示文本词条。 */
USTRUCT(BlueprintType, DisplayName = "显示文本词条")
struct FLxEntryDisplayText : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryDisplayText()
	{
		EntryType = ELxEntryType::DisplayText;
	}
};

/** 授予技能词条。 */
USTRUCT(BlueprintType, DisplayName = "授予技能词条")
struct FLxEntryGrantSkill : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryGrantSkill()
	{
		EntryType = ELxEntryType::GrantSkill;
	}

	/** 技能物品标签 ID。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "技能物品标签ID", meta = (Categories = "物品.技能"))
	FGameplayTag SkillItemIDTag;
};

/** 造成伤害词条。 */
USTRUCT(BlueprintType, DisplayName = "造成伤害词条")
struct FLxEntryDamage : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryDamage()
	{
		EntryType = ELxEntryType::Damage;
	}

	/** 生成伤害时读取的来源角色属性标签，例如攻击力。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条|伤害", DisplayName = "来源属性标签", meta = (Categories = "属性"))
	FGameplayTag SourceAttributeIDTag;

	/** 来源属性倍率，1.5 表示 150%，大于 1 为增加，小于 1 为减少。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条|伤害", DisplayName = "来源属性倍率（>1增加，<1减少）", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SourceAttributeRatio = 1.f;

	/** 生效后的伤害类型标签，例如普通伤害、火焰伤害或破甲伤害。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条|伤害", DisplayName = "生效伤害类型标签", meta = (Categories = "通用效果.伤害效果"))
	FGameplayTag DamageTypeTag;

	/** 伤害直接结算到的目标属性，留空时当前伤害流程默认按护盾和生命值结算。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条|伤害", DisplayName = "目标属性标签", meta = (Categories = "属性"))
	FGameplayTag TargetAttributeIDTag;
};

/**
 * 词条引用。
 *
 * 用于物品、Buff 等对象引用某一个具体词条。
 */
USTRUCT(BlueprintType, DisplayName = "词条引用")
struct FLxEntryQuote
{
	GENERATED_BODY()

	/** 词条标签 ID，用于引用全局词条缓存中的词条数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条", DisplayName = "词条标签ID", meta = (Categories = "词条"))
	FGameplayTag EntryID;

	/** 词条生效比例。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条生效比例")
	float EntryProportion = 1.f;

	/** 词条生效 CD。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "词条", DisplayName = "词条生效CD")
	float EntryCD = 1.f;

	FLxEntryQuote() {}
};

/**
 * 物品词条配置。
 *
 * 用于让物品上的每一条词条引用都对应一种逻辑类型，供后续物品修改功能判断可编辑规则。
 */
USTRUCT(BlueprintType, DisplayName = "物品词条配置")
struct FLxItemEntryConfig
{
	GENERATED_BODY()

	/** 物品上配置的词条引用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条", DisplayName = "词条引用")
	FLxEntryQuote EntryQuote;

	/** 该词条在物品修改、交互编辑等系统中的逻辑类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条", DisplayName = "词条逻辑类型")
	ELxEntryLogicType EntryLogicType = ELxEntryLogicType::Normal;

	FLxItemEntryConfig() {}

	FLxItemEntryConfig(const FLxEntryQuote& InEntryQuote, ELxEntryLogicType InEntryLogicType = ELxEntryLogicType::Normal)
		: EntryQuote(InEntryQuote)
		, EntryLogicType(InEntryLogicType)
	{
	}
};
