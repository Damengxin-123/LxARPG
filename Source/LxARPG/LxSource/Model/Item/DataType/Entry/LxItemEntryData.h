// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryEnum.h"
#include "LxARPG/LxSource/Core/Database/LxTableRowBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeSubData.h"
#include "LxItemEntryData.generated.h"

/**
 * @struct FLxItemEntry
 * @brief 表示物品词条的结构体
 *
 * 继承自FLxStructData，用于定义物品的词条属性。
 *
 * @note
 * - 本类型为所有物品统一词条，无论装备还是消耗，都需要继承此结构体建立单独的表
 * - 类型与角色属性枚举表关联，关联项为AppliedToAttRow属性，意为本词条将对此属性对应的角色属性进行更改
 * - 本类型ID将直接使用行名称
 */
USTRUCT(BlueprintType, DisplayName="物品词条属性")
struct FLxItemEntry : public FLxTableRowBase
{
	GENERATED_BODY()

	/* =============================== 数据 表内容 ==================================== */

	/**
	 * @var FText EntryName
	 * @brief 词条名称
	 *
	 * 用于UI显示内容。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条可视化", DisplayName="词条名称")
	FText EntryText;

	/**
     * @var FDataTableRowHandle EntryStyleRow
     * @brief 样式标签
     *
     * 用于在属性显示UI中显示文字样式的标签行。
     */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条可视化", DisplayName="词条样式标签",meta=(RowType="LxTextStyleData"))
	FDataTableRowHandle EntryStyleRow;


	/**
	 * @var FLxAttributeValueSet m_fEntryValue
	 * @brief 词条数值
	 * 词条通过约束属性的多种限制来实现更改属性，
	 * 在实际使用时，可以对属性约束条件的一条或多条进行加成
	 * 
	 * 该属性支持在编辑器中进行修改，并且可以通过蓝图读写操作访问。
	 * 用于设定物品词条的具体数值范围和浮动区间。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条属性", DisplayName="词条数值")
	FLxAttributeValueSet m_fEntryValue;
	
	/**
 	 * @var ELxItemEntryType EntryType
 	 * @brief 词条类型
 	 * 一个词条只能有一种类型
 	 * 定义词条的类型，如基础数值型、提高基础型等。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条属性", DisplayName="词条类型")
	ELxItemEntryType EntryType = ELxItemEntryType::BasicValue;
	

	
	/**
	 * @var FDataTableRowHandle AppliedToAttRow
	 * @brief 词条作用的属性ID
	 *
	 * 词条作用属性，直接引用属性表行的方式来关联角色属性。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条属性", DisplayName="词条作用的属性", meta=(RowType="LxAttributeInfo"))
	FDataTableRowHandle AppliedToAttRow;

	
	/* =============================== 逻辑运算内容 ==================================== */

	/**
	 * @var FName AppliedToAttID
	 * @brief 作用属性ID
	 *
	 * 表示此词条针对于什么属性进行改变，使用词条的逻辑将使用此属性。
	 */
	FName AppliedToAttID;

	/**
	 * @var FName EntryStyleName
	 * @brief 词条样式
	 *
	 * 词条的样式名称。
	 */
	FName EntryStyleName;

	/**
	 * @brief 词条数值乘法运算符重载
	 *
	 * 用于将词条数值乘以指定的强度值。
	 *
	 * @param ImproveValue 强度值，词条数值将乘以此值
	 * @return 返回修改后的词条引用
	 */
	FLxItemEntry& operator*(double ImproveValue)
	{
		m_fEntryValue *= ImproveValue;
		return *this;
	}

	virtual void InitData() override
	{
		// FLxRarityInfo* RarityInfoData = ItemRarityRow.GetRow<FLxRarityInfo>(TEXT("FLxItemAttributes::InitData"))
		// AppliedToAttID = AppliedToAttRow;
		// EntryStyleName = EntryStyleRow;
	};
};

/**
 * @struct FLxItemEntryName
 * @brief 用于在装备设计中简单的保存词条明目及数值比例
 *
 * @note
 * - 本类型仅用于设计装备及物品时添加词条，不可以单独创建数据表
 * - 本类型不需要ID
 * - 本类型关联词条表，意义为为装备添加具体的词条，并且设定强度
 */
USTRUCT(BlueprintType, DisplayName="词条列表项属性")
struct FLxItemEntryName : public FLxTableRowBase
{
	GENERATED_BODY()

	/**
	 * @var FDataTableRowHandle EntryRow
	 * @brief 词条ID
	 *
	 * 直接关联FLxItemEntry类型。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条|词条ID", DisplayName="词条ID", meta=(RowType="LxItemEntry"))
	FDataTableRowHandle EntryRow;

	/**
	 * @var double EntryStrength
	 * @brief 词条强度
	 *
	 * 相对于词条的标准数值的浮动比例。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条|词条强度", DisplayName="词条强度")
	double	EntryStrength = 1;


	FLxItemEntry m_fEntryValue;
	
	virtual void InitData() override
	{
		if(FLxItemEntry* RarityInfoData = EntryRow.GetRow<FLxItemEntry>(TEXT("FLxItemEntryName::InitData")))
		{
			m_fEntryValue =  *RarityInfoData;
		}
	}
};

