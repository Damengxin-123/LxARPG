// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxItemEntryCore.h"
#include "LxItemEntryEnum.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"
#include "LxItemEntryData.generated.h"


/**
 * @brief 物品词条定义类型
 *
 * 该结构体用于定义游戏中物品的词条信息，包括词条标签、作用目标标签、词条ID、可视化信息以及词条默认加成信息等。
 * 它继承自FTableRowBase，并被标记为BlueprintType，允许在蓝图中使用。
 */
USTRUCT(BlueprintType, DisplayName="物品词条定义类型")
struct FLxItemEntryDefine : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * @brief词条基础信息
	 *
	 * 该变量包含了关于词条的基础信息，如词条ID、词条类型以及相关的标签。它用于定义游戏中物品词条的核心属性，并且可以在编辑器中进行编辑和在蓝图中读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条基础信息", DisplayName="词条基础信息")
	FLxEnteryBaseInfo EnteryBaseInfo;

	/**
	 * @brief词条可视化描述信息
	 *
	 * 该变量包含了词条的可视化信息，包括词条名称和详细描述。它用于在游戏界面中显示词条的相关信息，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条可视化描述信息", DisplayName="词条可视化描述信息")
	FLxEnteryShowInfo EnteryShowInfo;

	/**
	 * @brief 角色属性类词条数据
	 *
	 * 该变量用于存储和定义角色属性相关的词条信息，包括具体的加成数值、数值比例等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `ChangeAttributeValue` 可以详细描述一个词条对角色属性的具体影响，例如攻击力、防御力等属性的加成效果。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性类词条数据", DisplayName="角色属性类词条数据")
	FLxChangeAttributeValue ChangeAttributeValue;

	/**
	 * @brief 角色状态类词条数据
	 *
	 * 该变量用于存储和定义与角色状态相关的词条信息，包括状态的改变、持续时间等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `ChangeStateValue` 可以详细描述一个词条对角色状态的具体影响，例如中毒、冰冻等状态效果。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色状态类词条数据", DisplayName="角色状态类词条数据")
	FLxChangeStateValue ChangeStateValue;

	/**
	 * @brief 创建buff类词条数据
	 *
	 * 该变量用于存储和定义创建buff相关的词条信息，包括buff的类型、持续时间、触发条件等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `CreaterBufferValue` 可以详细描述一个词条对创建buff的具体影响，例如生成的buff效果及其属性。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="创建buff类词条数据", DisplayName="创建buff类词条数据")
	FLxCreaterBufferValue CreaterBufferValue;

	/**
	 * @brief buff词条数据
	 *
	 * 该变量用于存储和定义与buff相关的词条信息，包括buff的属性、持续时间、触发条件等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `BufferEnterValue` 可以详细描述一个词条对buff的具体影响，例如buff的效果及其属性。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="buff词条数据", DisplayName="buff词条数据")
	FLxBufferEnterValue BufferEnterValue;

	/**
	 * @brief 描述文本类型词条数据
	 *
	 * 该变量用于存储和定义描述文本相关的词条信息，包括具体的文本内容、格式等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `TextEnterValue` 可以详细描述一个词条的文本信息，例如物品描述、技能说明等。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描述文本类型词条数据", DisplayName="描述文本类型词条数据")
	FLxTextEnterValue TextEnterValue;
};

/**
 * @brief 物品词条引用类型
 *
 * 该结构体用于引用游戏中物品的词条信息，允许通过数据表引用具体词条定义，并设置词条生效比例。
 * 它继承自FTableRowBase，并被标记为BlueprintType，允许在蓝图中使用。通过此结构体可以灵活地调整物品词条的效果强度。
 */
USTRUCT(BlueprintType, DisplayName="物品词条引用类型")
struct FLxItemEntryQuote
{
	GENERATED_BODY()

	/**
	 * @brief词条引用
	 *
	 * 该变量用于引用 `FLxItemEntryDefine` 数据表中的一个词条定义。通过这个引用，可以在游戏中获取和使用具体的物品词条信息。它支持在编辑器中进行编辑，并且可以在蓝图中读写。
	 *
	 * 该引用主要用于在游戏逻辑中查找并应用特定的物品词条，以便根据词条定义来影响游戏中的角色属性、状态等。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条引用", meta=(RowType="LxItemEntryDefine"))
	FDataTableRowHandle ItemEntryDefineTableQuote;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条生效比例")
	float EffectiveRatio = 1;
};

/**
 * @brief 物品词条缓存类型
 *
 * 该结构体用于缓存游戏中物品的词条信息，包括词条基础信息、可视化描述信息、角色属性类词条数据、角色状态类词条数据、创建buff类词条数据、buff词条数据以及描述文本类型词条数据等。
 * 它继承自FTableRowBase，并被标记为BlueprintType，允许在蓝图中使用。
 */
USTRUCT(BlueprintType, DisplayName="物品词条缓存类型")
struct FLxItemEntryData
{
	GENERATED_BODY()

		/**
	 * @brief词条基础信息
	 *
	 * 该变量包含了关于词条的基础信息，如词条ID、词条类型以及相关的标签。它用于定义游戏中物品词条的核心属性，并且可以在编辑器中进行编辑和在蓝图中读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条基础信息", DisplayName="词条基础信息")
	FLxEnteryBaseInfo EnteryBaseInfo;

	/**
	 * @brief词条可视化描述信息
	 *
	 * 该变量包含了词条的可视化信息，包括词条名称和详细描述。它用于在游戏界面中显示词条的相关信息，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="词条可视化描述信息", DisplayName="词条可视化描述信息")
	FLxEnteryShowInfo EnteryShowInfo;

	/**
	 * @brief 角色属性类词条数据
	 *
	 * 该变量用于存储和定义角色属性相关的词条信息，包括具体的加成数值、数值比例等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `ChangeAttributeValue` 可以详细描述一个词条对角色属性的具体影响，例如攻击力、防御力等属性的加成效果。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色属性类词条数据", DisplayName="角色属性类词条数据")
	FLxChangeAttributeValue ChangeAttributeValue;

	/**
	 * @brief 角色状态类词条数据
	 *
	 * 该变量用于存储和定义与角色状态相关的词条信息，包括状态的改变、持续时间等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `ChangeStateValue` 可以详细描述一个词条对角色状态的具体影响，例如中毒、冰冻等状态效果。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色状态类词条数据", DisplayName="角色状态类词条数据")
	FLxChangeStateValue ChangeStateValue;

	/**
	 * @brief 创建buff类词条数据
	 *
	 * 该变量用于存储和定义创建buff相关的词条信息，包括buff的类型、持续时间、触发条件等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `CreaterBufferValue` 可以详细描述一个词条对创建buff的具体影响，例如生成的buff效果及其属性。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="创建buff类词条数据", DisplayName="创建buff类词条数据")
	FLxCreaterBufferValue CreaterBufferValue;

	/**
	 * @brief buff词条数据
	 *
	 * 该变量用于存储和定义与buff相关的词条信息，包括buff的属性、持续时间、触发条件等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `BufferEnterValue` 可以详细描述一个词条对buff的具体影响，例如buff的效果及其属性。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="buff词条数据", DisplayName="buff词条数据")
	FLxBufferEnterValue BufferEnterValue;

	/**
	 * @brief 描述文本类型词条数据
	 *
	 * 该变量用于存储和定义描述文本相关的词条信息，包括具体的文本内容、格式等。它属于 `FLxItemEntryDefine` 结构体的一部分，并且可以在编辑器中进行编辑以及在蓝图中读写。
	 * 通过 `TextEnterValue` 可以详细描述一个词条的文本信息，例如物品描述、技能说明等。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描述文本类型词条数据", DisplayName="描述文本类型词条数据")
	FLxTextEnterValue TextEnterValue;

	/**
	 * @brief词条生效比例
	 *
	 * 该变量表示词条在实际应用中的生效比例。默认值为1，意味着词条效果将按照其定义的数值完全生效。
	 * 通过调整 `EffectiveRatio` 可以改变词条的效果强度，例如设置为0.5时，词条效果将减半。
	 * 该变量可以在编辑器中进行编辑，并且支持在蓝图中读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="词条生效比例")
	float EffectiveRatio = 1;
};
