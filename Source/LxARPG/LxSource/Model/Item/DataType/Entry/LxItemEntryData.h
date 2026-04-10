// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemEntryEnum.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxTextLineStyleData.h"
#include "LxItemEntryData.generated.h"

/**
 * @brief词条加成信息类型
 *
 * 该结构体用于存储和管理词条的加成信息，包括数值上限、当前有效值、浮动比例以及加成方式和对象。
 * 通过编辑器或蓝图可以对这些属性进行读写操作。此结构体在编辑器中显示为“词条加成信息类型”。
 *
 * @note 在设置相关属性时，请确保数值合理且与加成方式及对象匹配，以避免逻辑错误。
 */
USTRUCT(BlueprintType, DisplayName="词条加成信息类型")
struct FLxItemEntryValueInfo
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
	int32 ValueLimit = 0;
	/**
	 * @brief 当前有效值
	 *
	 * 该变量表示当前属性的有效数值。可以通过编辑器或蓝图对其进行读写操作。
	 * 在编辑器中，该变量显示为“当前有效值”。
	 *
	 * @note 确保设置的数值在合理范围内，以避免逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="当前有效值")
	int32 Value = 0;
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
	 * @brief 加成方式
	 *
	 * 该变量定义了属性加成的具体方式。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“加成方式”。
	 *
	 * 可选的加成方式包括：
	 * - 基础数值型：表示可以直接加算到角色属性值上。
	 * - 提高基础型：表示可以提高基础值的百分比，仅用于基础属性。
	 * - 额外提高型：表示在当前属性计算结果上，再次进行百分比提高，作用于当前属性数值。
	 * - 机制型：表示为角色开启或关闭某些机制。
	 *
	 * @note 根据实际需求选择合适的加成方式，并确保其与加成对象（EntryTarget）相匹配，以避免逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="加成方式")
	ELxItemEntryType EntryType = ELxItemEntryType::BasicValue;

	/**
	 * @brief 加成对象
	 *
	 * 该变量定义了属性加成的目标对象。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“加成对象”。
	 *
	 * 可选的加成对象包括：
	 * - 作用于值的上限：表示加成应用于数值上限。
	 * - 作用于有效值：表示加成应用于当前有效值。
	 * - 作用于向上浮动比例：表示加成应用于向上浮动比例。
	 * - 作用于向下浮动比例：表示加成应用于向下浮动比例。
	 *
	 * @note 根据实际需求选择合适的加成对象，并确保其与加成方式（EntryType）相匹配，以避免逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="加成对象")
	ELxItemEntryTarget EntryTarget = ELxItemEntryTarget::ToValue;
};


/**
 * @brief 物品词条定义类型
 *
 * 该结构体用于定义和管理游戏中的物品词条信息，包括词条ID、名称、描述以及样式和属性引用等。
 * 通过编辑器或蓝图可以对这些属性进行读写操作。此结构体在编辑器中显示为“物品词条定义类型”。
 *
 * @note 在设置相关属性时，请确保所有引用的数据表行类型正确无误，以避免逻辑错误。
 */
USTRUCT(BlueprintType, DisplayName="物品词条定义类型")
struct FLxItemEntryDefine : public FTableRowBase
{
	GENERATED_BODY()


	/**
	 * @brief词条ID
	 *
	 * 该变量用于唯一标识一个词条。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条ID”。
	 *
	 * @note 确保词条ID的唯一性，以避免在数据引用时发生冲突。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条ID")
	FName EntryID;

	/**
	 * @brief 词条名称
	 *
	 * 该变量用于存储和显示词条的名称。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条名称”。
	 *
	 * @note 确保词条名称具有描述性和唯一性，以便于在项目中进行识别和引用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条可视化信息", DisplayName="词条名称")
	FText DisplayName;
	/**
	 * @brief 词条描述信息
	 *
	 * 该变量用于存储和显示词条的详细描述文本。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条描述信息”。
	 *
	 * @note 确保词条描述信息清晰且准确，以便于在项目中理解和使用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条可视化信息", DisplayName="词条描述信息")
	FText Description;

	/**
	 * @brief词条显示样式引用
	 *
	 * 该变量用于引用词条的显示样式数据。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条显示样式引用”。
	 *
	 * @note 确保引用的数据表行类型为 `LxTextLineStyleData`，以确保正确的样式信息被应用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条显示样式引用", meta=(RowType="LxTextLineStyleData"))
	FDataTableRowHandle TextStyleTableQuote;
	/**
	 * @brief词条作用属性引用
	 *
	 * 该变量用于引用词条所作用的属性定义信息。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条作用属性引用”。
	 *
	 * @note 确保引用的数据表行类型为 `LxAttributeDefineInfo`，以确保正确的属性定义信息被应用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条作用属性引用", meta=(RowType="LxAttributeDefineInfo"))
	FDataTableRowHandle AttributeDefineTableQuote;
	/**
	 * @brief 词条默认加成信息
	 *
	 * 该变量存储了词条的默认加成信息，包括数值上限、当前有效值、浮动比例以及加成方式和对象。
	 * 通过编辑器或蓝图可以对这些属性进行读写操作。此变量在编辑器中显示为“词条默认加成信息”。
	 *
	 * @note 在设置相关属性时，请确保数值合理且与加成方式及对象匹配，以避免逻辑错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条默认加成信息")
	FLxItemEntryValueInfo ItemEntryDefineValue;
};


/**
 * @brief 物品词条引用类型
 *
 * 该结构体用于存储和管理物品的词条引用数据，包括词条定义表中的特定行引用以及词条生效比例。
 * 用于在设计物品时，快速添加物品的词条
 *
 * @note 在设置相关属性时，请确保引用的词条定义存在且有效，并且提供的数值合理，以避免运行时错误或游戏平衡性问题。
 */
USTRUCT(BlueprintType, DisplayName="物品词条引用类型")
struct FLxItemEntryQuote
{
	GENERATED_BODY()
	/**
	 * @brief 词条引用 *
	 * 该属性用于引用词条定义表中的特定行，以便在物品词条数据中使用。通过编辑器或蓝图可以对此属性进行读写操作。
	 * 引用的词条定义包含了词条的具体信息和行为，此属性在编辑器中显示为“词条引用”。
	 *
	 * @note 确保引用的词条定义存在且有效，以避免运行时错误。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条引用", meta=(RowType="LxItemEntryDefine"))
	FDataTableRowHandle ItemEntryDefineTableQuote;

	/**
	 * @brief词条生效比例
	 *
	 * 该浮点数值表示词条效果生效时的浮动比例，用于调整词条效果的实际强度。默认值为1，意味着词条将按其定义的效果完全生效。
	 * 通过编辑器或蓝图可以对此属性进行读写操作，以动态改变词条效果的强度。此属性在编辑器中显示为“词条生效比例”。
	 *
	 * @note 当设置此属性时，请确保提供的数值合理，避免因过大的浮动比例导致游戏平衡性问题。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条生效比例")
	float UpwardFloatingRatio = 1;
};
/**
 * @brief 物品词条缓存类型
 * 此类型用于在装备等物品初始化时，读取物品词条引用后，存放所有词条信息
 * 该结构体用于存储和管理物品词条的相关数据，包括词条ID、名称、描述信息等。通过编辑器或蓝图可以对这些属性进行读写操作。
 * 
 *
 * @note 确保所有字段的设置合理且一致，以保证数据的有效性和一致性。
 */
USTRUCT(BlueprintType, DisplayName="物品词条缓存类型")
struct FLxItemEntryData
{
	GENERATED_BODY()
	/**
 	 * @brief词条ID
 	 *
 	 * 该变量用于唯一标识一个词条。通过编辑器或蓝图可以对其进行读写操作。
 	 * 在编辑器中，该变量显示为“词条ID”。
 	 *
 	 * @note 确保词条ID的唯一性，以避免在数据引用时发生冲突。
 	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条ID")
	FName EntryID;
	
	/**
	 * @brief 词条名称
	 *
	 * 该变量用于存储和显示词条的名称。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条名称”。
	 *
	 * @note 确保词条名称具有描述性和唯一性，以便于在项目中进行识别和引用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条可视化信息", DisplayName="词条名称")
	FText DisplayName;
	/**
	 * @brief 词条描述信息
	 *
	 * 该变量用于存储和显示词条的详细描述文本。通过编辑器或蓝图可以对其进行读写操作。
	 * 在编辑器中，该变量显示为“词条描述信息”。
	 *
	 * @note 确保词条描述信息清晰且准确，以便于在项目中理解和使用。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "词条可视化信息", DisplayName="词条描述信息")
	FText Description;

	// 词条样式指针
	FLxTextLineStyleData*  TextStyle = nullptr;
	// 词条作用属性ID
	FName AttributeID;
	// 词条值
	FLxItemEntryValueInfo ItemEntryDefineValue;
	// 词条生效比例值
	float EffectiveRatio = 1;
};

