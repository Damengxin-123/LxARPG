// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"
#include "UObject/Object.h"
#include "LxAttributeCoreType.generated.h"

/**
 * @brief 属性基础信息
 *
 * 该结构体用于存储属性的基础信息，包括属性的唯一ID和类型。通过蓝图或编辑器可对其进行读写操作。
 * 在编辑器中，该结构体显示为“属性基础信息”。
 *
 * @note 确保在设置此结构体的属性时使用有效的值以避免冲突。
 */
USTRUCT(BlueprintType, DisplayName="属性基础信息")
struct FLxAttributeInfo
{
	GENERATED_BODY()

	/**
	 * @brief 属性唯一ID
	 *
	 * 该变量用于标识属性的唯一名称。通过蓝图或编辑器可对其进行读写操作。
	 * 在编辑器中，该字段显示为“属性唯一ID”。
	 *
	 * @note 确保在设置此属性时使用唯一的FName值以避免冲突。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, DisplayName="属性唯一ID")
	FName AttributeID;

	/**
	 * @brief 属性类型
	 *
	 * 该变量用于标识属性的类型。通过蓝图或编辑器可对其进行读写操作。
	 * 在编辑器中，该字段显示为“属性类型”。
	 *
	 * 可选的属性类型包括基础属性、攻击属性、防御属性、元素属性、信仰属性、其他属性和无属性。
	 *
	 * @note 确保在设置此属性时使用有效的ELxAttributeType枚举值。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, DisplayName="属性类型")
	ELxAttributeType AttributeType = ELxAttributeType::None;
};

/**
 * @brief 属性可视化信息
 *
 * 该结构体用于定义属性在用户界面中的显示方式，如编辑器或游戏内UI。通过蓝图或编辑器可对其进行配置。
 * 在编辑器中，该结构体显示为“属性可视化信息”。
 *
 * @note 确保合理设置此结构体的属性以提供清晰且有用的用户界面展示。
 */
USTRUCT(BlueprintType, DisplayName="属性可视化信息")
struct FLxAttributeShowInfo
{
	GENERATED_BODY()
	/** FLxRichTextDescriptionData
 	 * @var FText m_strAttName
 	 * @brief 属性名称
 	 *
 	 * 用于UI显示，需要进行多语言化。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性可视化名称")
	FLxRichTextDescriptionGroupData   AttributeName;

	/**
	 * @brief 属性可视化描述
	 *
	 * 该字段用于存储属性的详细描述信息，适用于UI显示或文档说明。通过蓝图或编辑器可以对其进行读写操作。
	 * 在编辑器中，此字段显示为“属性可视化描述”。
	 *
	 * @note 确保提供的描述信息准确且符合项目需求，以便于开发者和用户理解。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性可视化描述")
	FText   AttributeDescription;
	/**
   	 * @var bool m_bIsVisible
   	 * @brief 是否显示
   	 *
   	 * 控制属性是否在列表中依次显示。
   	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="是否在列表中依次显示")
	bool	IsVisible = false;
};

/**
 * @brief 角色属性值信息
 *
 * 该结构体用于存储角色属性的具体数值信息，包括数值上限、当前有效值、浮动比例及属性值类型。通过蓝图或编辑器可对其进行读写操作。
 * 在编辑器中，该结构体显示为“角色属性值信息类型”。
 *
 * @note 确保在设置此结构体的属性时使用有效的值以避免逻辑错误。
 */
USTRUCT(BlueprintType, DisplayName="角色属性值信息类型")
struct FLxAttributeValue
{
	GENERATED_BODY()
	/**
	 * @brief 数值上限
	 *
	 * 该变量定义了属性数值的最大限制。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“数值上限”。
	 *
	 * @note 确保设置的数值上限合理，以避免超出预期范围导致的逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="数值上限")
	float ValueLimit = 0;
	/**
	 * @brief 当前有效值
	 *
	 * 该变量表示当前属性的有效数值。可以通过编辑器或蓝图对其进行读写操作。
	 * 在编辑器中，该变量显示为“当前有效值”。
	 *
	 * @note 确保设置的数值在合理范围内，以避免逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="当前有效值")
	float Value = 0;
	/**
	 * @brief 向上浮动比例
	 *
	 * 该变量定义了属性值向上浮动的比例。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“向上浮动比例”。
	 *
	 * @note 默认值为1，表示无浮动。设置大于1的值将使属性值增加，而小于1的值将减小属性值。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="向上浮动比例")
	float UpwardFloatingRatio = 1;
	/**
	 * @brief 向下浮动比例
	 *
	 * 该变量定义了属性值向下浮动的比例。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“向下浮动比例”。
	 *
	 * @note 默认值为1，表示无浮动。设置小于1的值将使属性值减少，而大于1的值将增加属性值。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="向下浮动比例")
	float DownwardFloatingRatio = 1;
	/**
	 * @brief 角色属性值类型
	 *
	 * 该枚举变量定义了角色属性值的类型，用于区分不同的数值处理方式。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“角色属性值类型”。
	 *
	 * @note 确保选择合适的属性值类型以匹配实际需求，避免逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="角色属性值类型")
	ELxCharacterValueType ValueType = ELxCharacterValueType::FixedNumeric;
	
};
