// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxAttributeCoreType.generated.h"

USTRUCT(BlueprintType, DisplayName="属性基础信息")
struct FLxAttributeInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, DisplayName="属性唯一ID")
	ELxCharacterAttributeID AttributeID = ELxCharacterAttributeID::X_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, DisplayName="属性类型")
	ELxAttributeType AttributeType = ELxAttributeType::None;

	/**
	 * Tags used by entries, buffs, equipment and skills to decide whether this attribute is a valid target.
	 * Example: Strength can use Module.Attribute, Attribute.Strength and Trait.BasicAttribute.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, DisplayName="属性标签", meta=(Categories="Module,Attribute,Function,Trait,SkillForm"))
	FGameplayTagContainer AttributeTags;

	FLxAttributeInfo()
	{
		AttributeTags.AddTag(LxTag_Module_Attribute);
	}
};

USTRUCT(BlueprintType, DisplayName="属性可视化信息")
struct FLxAttributeShowInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性可视化名称")
	FLxRichTextDescriptionGroupData AttributeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性可视化描述")
	FText AttributeDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="是否在列表中依次显示")
	bool IsVisible = false;
};

USTRUCT(BlueprintType, DisplayName="角色属性值信息类型")
struct FLxAttributeValue
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="角色属性值类型")
	ELxCharacterValueType ValueType = ELxCharacterValueType::FixedNumeric;
};
