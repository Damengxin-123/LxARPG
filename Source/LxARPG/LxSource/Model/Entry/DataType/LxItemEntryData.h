// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LxEntryEnum.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Item/DataType/ShowInfoConfig/LxItemShowInfoConfigID.h"
#include "LxItemEntryData.generated.h"

/**
 * 词条可视化文本。
 *
 * 用于在 UI 中展示词条名称和描述。
 */
USTRUCT(BlueprintType, DisplayName="词条可视化文本")
struct FLxEntryText
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="词条可视化名称")
	FText EntryDisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="词条可视化描述")
	FText EntryDescribeText;
};

/**
 * 词条基础数据。
 *
 * 所有具体词条数据表结构体的基类。
 */
USTRUCT(BlueprintType, DisplayName="词条基础数据")
struct FLxEntryBase : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="词条类型")
	ELxEntryType EntryType = ELxEntryType::NoneEntryType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="词条可视化信息")
	FLxEntryText EntryText;
};

/**
 * 属性增益词条。
 */
USTRUCT(BlueprintType, DisplayName="属性增益词条")
struct FLxEntryAttributeGain : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryAttributeGain()
	{
		EntryType = ELxEntryType::AttributeGain;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="属性增益词条ID")
	ELxAttributeGainEntryID EntryID = ELxAttributeGainEntryID::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="作用属性ID")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="作用对象")
	ELxEntryTarget EntryTarget = ELxEntryTarget::ToValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="作用方式")
	ELxEntryEffectiveType EffectiveType = ELxEntryEffectiveType::BasicValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="词条数值")
	float EntryValue = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="目标标签")
	FGameplayTagContainer TargetTags;
};

/**
 * 属性回复词条。
 */
USTRUCT(BlueprintType, DisplayName="属性回复词条")
struct FLxEntryAttributeRecovery : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryAttributeRecovery()
	{
		EntryType = ELxEntryType::AttributeRecovery;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="属性回复词条ID")
	ELxAttributeRecoveryEntryID EntryID = ELxAttributeRecoveryEntryID::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="作用属性ID")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="作用方式")
	ELxEntryEffectiveType EffectiveType = ELxEntryEffectiveType::BasicValue;

	// 如果没有持续时间，则是一次回复的数值，如果有持续时间，则是每秒的恢复的数值
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="回复数值")
	float EntryValue = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="目标标签")
	FGameplayTagContainer TargetTags;
};

/**
 * 状态改变词条。
 */
USTRUCT(BlueprintType, DisplayName="状态改变词条")
struct FLxEntryChangeState : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryChangeState()
	{
		EntryType = ELxEntryType::ChangeState;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="状态改变词条ID")
	ELxChangeStateEntryID EntryID = ELxChangeStateEntryID::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="状态ID")
	uint8 StateID = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="状态词条值")
	ELxEntryStateValue StateValue = ELxEntryStateValue::Add;
};

/**
 * 创建 Buff 词条。
 */
USTRUCT(BlueprintType, DisplayName="创建Buff词条")
struct FLxEntryCreateBuff : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryCreateBuff()
	{
		EntryType = ELxEntryType::CreateBuff;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="创建Buff词条ID")
	ELxCreateBuffEntryID EntryID = ELxCreateBuffEntryID::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="BuffID")
	int32 BuffID = ItemIDNone;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="持续时间")
	float BuffDuration = -1.f;
};

/**
 * 多目标词条。
 */
USTRUCT(BlueprintType, DisplayName="多目标词条")
struct FLxEntryMultiTarget : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryMultiTarget()
	{
		EntryType = ELxEntryType::MultiTarget;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="多目标词条ID")
	ELxMultiTargetEntryID EntryID = ELxMultiTargetEntryID::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="目标标签")
	FGameplayTagContainer TargetTags;
};

/**
 * 显示文本词条。
 */
USTRUCT(BlueprintType, DisplayName="显示文本词条")
struct FLxEntryDisplayText : public FLxEntryBase
{
	GENERATED_BODY()

	FLxEntryDisplayText()
	{
		EntryType = ELxEntryType::DisplayText;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Entry", DisplayName="显示文本词条ID")
	ELxDisplayTextEntryID EntryID = ELxDisplayTextEntryID::None;
};

/**
 * 词条引用。
 *
 * 用于物品、Buff 等对象引用某个具体词条。
 */
USTRUCT(BlueprintType, DisplayName="词条引用")
struct FLxEntryQuote
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条", DisplayName="词条引用", meta=(RowType="LxEntryBase"))
	FDataTableRowHandle EntryQuote;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="词条", DisplayName="词条生效比例")
	float EntryProportion = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="词条", DisplayName="词条生效CD")
	float EntryCD = 1.f;

	FLxEntryQuote() {}

};