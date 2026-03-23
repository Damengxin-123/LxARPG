/**
 * @file LxAttributeData.h
 * @brief 角色属性数据定义
 *
 * 此文件定义了角色属性系统相关的数据结构，包括属性加成、属性信息和属性集合等。
 */

#pragma once

#include "LxAttributeEnumType.h"
#include "CoreMinimal.h"
#include "LxAttributeSubData.h"
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
USTRUCT(BlueprintType)
struct FLxAttributeInfo : public FLxTableRowBase
{
	GENERATED_BODY()

	/**
	 * @var ELxAttributeType m_nAttType
	 * @brief 属性类型
	 *
	 * 用于设置显示的标签，区分不同类型的属性。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性|属性值信息设置", DisplayName="属性类型")
	ELxAttributeType m_nAttType = ELxAttributeType::None;

	/**
	 * @var ELxCharacterValueType m_nAttValueType
	 * @brief 属性值类型
	 *
	 * 用于区分属性值是数值型还是机制型。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性|属性值信息设置", DisplayName="属性值类型")
	ELxCharacterValueType m_nAttValueType = ELxCharacterValueType::FixedNumeric;

	/**
	 * @var FText m_strAttName
	 * @brief 属性名称
	 *
	 * 用于UI显示，需要进行多语言化。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性|属性可视化设置", DisplayName="属性可视化名称")
	FText   m_strAttText;

	/**
	 * @var bool m_bIsVisible
	 * @brief 是否显示
	 *
	 * 控制属性是否在列表中依次显示。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性|属性可视化设置", DisplayName="是否在列表中依次显示")
	bool	m_bIsVisible = true;

	/**
	 * @var FDataTableRowHandle m_tabAttStyle
	 * @brief 样式标签
	 *
	 * 用于在属性显示UI中显示文字样式的标签行。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性|属性可视化设置", DisplayName="文本行样式",meta=(RowType="LxTextStyleData"))
	FDataTableRowHandle m_tabAttStyle;
};

/**
 * @struct FLxAttributeSet
 * @brief 角色属性设置表
 * 用于实际存放不同角色的属性定义的结构体
 * 用于存储和管理角色的单个属性，包括基础值、各种加成值和最终计算值。
 * 继承自FLxStructData，支持数据表配置。
 *
 * @note 可在蓝图中使用
 */
USTRUCT(BlueprintType)
struct FLxAttributeSet : public FLxTableRowBase
{
	GENERATED_BODY()

	/* ======================================== 设计阶段值 =================================================== */
	/**
	 * @var FDataTableRowHandle m_tabAttInfoQuote
	 * @brief 引用的属性定义表
	 *
	 * 用于引用属性信息表中的具体属性配置。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性设置", DisplayName="属性表引用",meta=(RowType="LxAttributeInfo"))
	FDataTableRowHandle m_tabAttInfoQuote;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色属性值设置", DisplayName="默认值设置")
	FLxAttributeValueSet m_fDefaultValueSet;

	/* ======================================== 逻辑运算内容 =================================================== */

	/**
	 * @var FLxAttributeValueSet m_fCurrentValueSet
	 * @brief 当前属性值集合
	 *
	 * 该变量用于存储经过计算后的当前有效属性值设置。这些属性值是在考虑了默认值、装备加成、buff加成、天赋加成和技能加成等因素后得出的最终结果。
	 * 它包含了最大值、最小值以及浮动比例等信息，用于在游戏逻辑中进行各种属性相关的计算。
	 * 计算规则：
	 *		1、先计算基础数值增减，然后计算提高，最后计算总增；
	 *		2、对于机制型数值，全部为开启则最终为开启，否则为关闭；
	 *		3、对于属性值浮动比例，在计算式选取一个加成中最大的作为最终比例。
	 *
	 * @note 支持蓝图读写操作
	 */
	FLxAttributeValueSet m_fCurrentValueSet;

	/**
	 * @var AttValueType m_nCurrentValue
	 * @brief 当前属性的有效值
	 *
	 * 当前属性有效值存储 用于存储经过计算之后的属性值的有效值
	 * 属性有效值指在参与系统计算时使用的值，
	 * 例如当前生命值，他会受到最大最小值限制，例如最大生命值和最小生命之限制
	 * 也会受浮动比例影响，产生例如攻击力：8 ~ 15这样的属性值区间 
	 *
	 * @note 支持蓝图读写操作
	 */
	int32 m_nCurrentValue;

	/**
	 * @var FLxAttributeInfo m_sAttInfo
	 * @brief 当前属性设置关联的属性定义表
	 *
	 * 在初始化时，为其设置属性信息表中同名结构体。
	 */
	FLxAttributeInfo m_fAttInfoData;

	/**
	 * @brief 初始化数据
	 *
	 * 在结构体创建时，初始化某些值，例如：当使用数据表值填充此结构体后，需要从引用的表行中读取数据等。
	 */
	virtual void InitData() override
	{
		if (!m_tabAttInfoQuote.IsNull())
		{
			if (const FLxAttributeInfo* RowData = m_tabAttInfoQuote.GetRow<FLxAttributeInfo>(TEXT("Read ItemRowHandle")))
			{
				m_fAttInfoData = *RowData;
			}
		}
		m_fCurrentValueSet = m_fDefaultValueSet;
		m_nCurrentValue = m_fCurrentValueSet.m_nMaxValue;
	}
};


