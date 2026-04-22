// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxItemEntryEnum.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"
#include "LxItemEntryData.generated.h"

USTRUCT(BlueprintType, DisplayName="词条加成信息类型")
struct FLxItemEntryValueInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="数值上限")
	float ValueLimit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="当前有效值")
	float Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="向上浮动比例")
	float UpwardFloatingRatio = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="向下浮动比例")
	float DownwardFloatingRatio = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="加成方式")
	ELxItemEntryType EntryType = ELxItemEntryType::BasicValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="加成对象")
	ELxItemEntryTarget EntryTarget = ELxItemEntryTarget::ToValue;
};

USTRUCT(BlueprintType, DisplayName="物品词条定义类型")
struct FLxItemEntryDefine : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="标签", DisplayName="词条标签", meta=(Categories="Module,Attribute,Function,Trait,SkillForm"))
	FGameplayTagContainer EntryTags;

	FLxItemEntryDefine()
	{
		EntryTags.AddTag(LxTag_Module_Entry);
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条ID")
	FName EntryID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条可视化信息", DisplayName="词条名称")
	FLxRichTextDescriptionGroupData DisplayNameData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条可视化信息", DisplayName="词条描述信息")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条作用属性")
	ELxCharacterAttributeID AttributeTypeID = ELxCharacterAttributeID::X_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条默认加成信息")
	FLxItemEntryValueInfo ItemEntryDefineValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条逻辑类型")
	ELxItemEntryLogicType ItemEntryLogicType = ELxItemEntryLogicType::None;
};

USTRUCT(BlueprintType, DisplayName="物品词条引用类型")
struct FLxItemEntryQuote
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条引用", meta=(RowType="LxItemEntryDefine"))
	FDataTableRowHandle ItemEntryDefineTableQuote;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条生效比例")
	float UpwardFloatingRatio = 1;
};

USTRUCT(BlueprintType, DisplayName="物品词条缓存类型")
struct FLxItemEntryData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="标签", DisplayName="词条标签", meta=(Categories="Module,Attribute,Function,Trait,SkillForm"))
	FGameplayTagContainer EntryTags;

	FLxItemEntryData()
	{
		EntryTags.AddTag(LxTag_Module_Entry);
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条ID")
	FName EntryID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条可视化信息", DisplayName="词条名称")
	FLxRichTextDescriptionGroupData DisplayNameData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条可视化信息", DisplayName="词条描述信息")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条作用属性")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条数值")
	FLxItemEntryValueInfo ItemEntryDefineValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条逻辑类型")
	ELxItemEntryLogicType ItemEntryLogicType = ELxItemEntryLogicType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条生效比例")
	float EffectiveRatio = 1;
};
