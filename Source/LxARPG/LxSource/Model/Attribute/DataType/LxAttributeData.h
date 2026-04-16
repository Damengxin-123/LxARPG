/**
 * @file LxAttributeData.h
 * @brief 角色属性数据定义
 *
 * 此文件定义了角色属性系统相关的数据结构，包括属性加成、属性信息和属性集合等。
 */

#pragma once

#include "LxAttributeCoreType.h"
#include "CoreMinimal.h"
#include "../../Style/DataType/LxTextLineStyleData.h"
#include "LxARPG/LxSource/Core/Database/LxTableRowBase.h"
#include "LxAttributeData.generated.h"

/**
 * @struct FLxAttributeInfo
 * @brief 用于定义角色属性的类型、值类型、名称等信息
 *
 * 该结构体继承自FLxStructData，提供了属性类型、属性值类型、显示名称、可见性以及样式标签的设置。
 * 用于配置所有生物单位的属性，在设置具体生物类型的属性时，需要引用此表进行数值设置。
 *
 * @note 此表仅用于在编辑器中配置属性，不可直接使用。可在蓝图中使用。
 */
USTRUCT(BlueprintType, DisplayName="角色属性定义类型")
struct FLxAttributeDefineInfo : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * @property FLxAttributeInfo AttributeInfo
	 * @brief 用于存储和配置属性的基础信息
	 *
	 * 该属性包含了角色属性的唯一ID和类型，是定义角色具体属性时不可或缺的部分。
	 * 通过编辑器或蓝图可以对其进行读写操作，以便于根据需要调整角色属性的基本设置。
	 *
	 * 在编辑器中，该字段显示为“属性基础信息”。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性基础信息")
	FLxAttributeInfo AttributeInfo;

	/**
	 * @var FLxAttributeShowInfo AttributeShowInfo
	 * @brief 用于配置属性在UI中的可视化信息
	 *
	 * 该变量包含属性的显示名称、描述以及是否可见等信息，用于控制属性在用户界面中的展示方式。
	 * 可以通过编辑器或蓝图进行读写操作，以自定义属性的可视化设置。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性可视化信息")
	FLxAttributeShowInfo AttributeShowInfo;
	
};

/**
 * @struct FLxAttributeDefineValue
 * @brief 用于定义角色属性的具体值及其引用信息
 *
 * 继承自FLxTableRowBase，该结构体主要用于配置角色的特定属性值。通过引用属性定义表和设置默认值，为角色提供详细的属性配置。
 * 在编辑器中，此结构体可用于配置角色属性的具体数值，并支持在蓝图中的读写操作。
 *
 * @note 此结构体仅用于在编辑器中配置属性，不可直接使用。可在蓝图中使用。
 */
USTRUCT(BlueprintType, DisplayName="角色属性值设计类型")
struct FLxAttributeDefineValue : public FTableRowBase
{
	GENERATED_BODY()
	/**
	 * @var FDataTableRowHandle AttributeTableQuote
	 * @brief 角色属性定义引用
	 *
	 * 该变量是一个数据表行句柄，用于引用角色属性定义信息。通过此句柄可以访问和操作与角色属性相关的数据表。
	 * 在编辑器中，该变量显示为“角色属性定义引用”。
	 *
	 * @note 此引用仅用于在编辑器中配置属性，不可直接使用。可在蓝图中使用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="角色属性定义引用",meta=(RowType="LxAttributeDefineInfo"))
	FDataTableRowHandle AttributeTableQuote;

	/**
	 * @var FLxAttributeValue AttributeValue
	 * @brief 角色属性默认值设定
	 *
	 * 该变量用于设置角色属性的默认值，包括数值上限、当前有效值、向上浮动比例、向下浮动比例以及属性值类型。
	 * 通过编辑器或蓝图可以对其进行读写操作。在编辑器中，该变量显示为“角色属性默认值设定”。
	 *
	 * @note 确保设置的数值在合理范围内，以避免逻辑错误。此变量主要用于配置角色属性的默认值。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="角色属性默认值设定")
	FLxAttributeValue AttributeValue;
};

/**
 * @struct FLxAttributeData
 * @brief 角色属性值缓存类型
 *
 * 该结构体用于存储和管理角色属性的相关信息，包括属性的基础信息、可视化信息、文本样式、默认值以及计算后的值。
 * 通过编辑器或蓝图可以对其进行读写操作，以自定义角色属性的各个方面。
 *
 * @note 此结构体主要用于在编辑器中配置角色属性，不可直接使用。可在蓝图中使用。
 */
USTRUCT(BlueprintType, DisplayName="角色属性值缓存类型")
struct FLxAttributeData
{
	GENERATED_BODY()
	/**
	 * @var FLxAttributeInfo AttributeInfo
	 * @brief 用于存储属性的基础信息
	 *
	 * 该变量包含属性的唯一ID和类型，是定义角色属性时的基础配置项。
	 * 通过编辑器或蓝图可以对其进行读写操作，以自定义属性的基本设置。
	 *
	 * @note 确保在设置此变量时使用唯一的FName值作为属性ID，并使用有效的ELxAttributeType枚举值作为属性类型。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性基础信息")
	FLxAttributeInfo AttributeInfo;

	/**
 * @var FLxAttributeShowInfo AttributeShowInfo
 * @brief 用于配置属性在UI中的可视化信息
 *
 * 该变量包含属性的显示名称、描述以及是否可见等信息，用于控制属性在用户界面中的展示方式。
 * 可以通过编辑器或蓝图进行读写操作，以自定义属性的可视化设置。
 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="属性可视化信息")
	FLxAttributeShowInfo AttributeShowInfo;

	/**
 	 * @var FLxAttributeValue AttributeValue
 	 * @brief 角色属性默认值设定
 	 *
 	 * 该变量用于设置角色属性的默认值，包括数值上限、当前有效值、向上浮动比例、向下浮动比例以及属性值类型。
 	 * 通过编辑器或蓝图可以对其进行读写操作。在编辑器中，该变量显示为“角色属性默认值设定”。
 	 *
 	 * @note 确保设置的数值在合理范围内，以避免逻辑错误。此变量主要用于配置角色属性的默认值。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="角色属性默认值")
	FLxAttributeValue AttributeValue;

	/**
	 * @var FLxAttributeValue CalculatedAttributeValue
	 * @brief 角色属性计算后的值
	 *
	 * 该变量存储了角色属性经过计算后的最终值，包括数值上限、当前有效值、向上浮动比例、向下浮动比例以及属性值类型。
	 * 通过编辑器或蓝图可以对其进行读写操作。在编辑器中，该变量显示为“角色属性计算后的值”。
	 *
	 * @note 计算后的值是基于默认值和其他影响因素（如装备加成、状态效果等）综合得出的，确保其在合理范围内以避免逻辑错误。
	 */
	FLxAttributeValue CalculatedAttributeValue;
};



