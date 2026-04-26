#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxItemEntryLogic.generated.h"

/**
 * @class ULxItemEntryLogic
 * @brief 词条逻辑类型，用于处理和管理游戏中的词条数据。
 *
 * 该类提供了创建、初始化词条逻辑对象的方法，并且支持获取词条的显示名称、显示文本以及最终加成数值等功能。词条逻辑对象通常与装备、角色属性等系统关联，以实现对游戏中各种属性的动态调整。
 */
UCLASS(BlueprintType, Blueprintable, DisplayName="词条逻辑类型")
class LXARPG_API ULxItemEntryLogic : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建一个新的ULxItemEntryLogic对象，并初始化它。
	 *
	 * 该函数尝试创建一个新的ULxItemEntryLogic实例，使用给定的`InOuter`作为外部对象。新创建的对象随后会使用`InEntryQuote`参数进行初始化。如果`InOuter`为nullptr或者初始化失败，则返回nullptr。
	 *
	 * @param InEntryQuote 用于初始化新创建的ULxItemEntryLogic对象的词条引用数据。
	 * @param InOuter 新创建的ULxItemEntryLogic对象将隶属于这个外部对象。如果此参数为nullptr，则无法创建新的逻辑对象。
	 * @return 如果成功创建并初始化了ULxItemEntryLogic对象，则返回指向该对象的指针；否则返回nullptr。
	 */
	static ULxItemEntryLogic* CreateItemEntryLogicObject(const FLxItemEntryQuote& InEntryQuote, UObject* InOuter);

	/**
	 * @brief 初始化词条逻辑对象。
	 *
	 * 该函数用于初始化ULxItemEntryLogic对象，使用提供的词条引用数据`InEntryQuote`来设置内部的词条数据。初始化过程包括从数据表中获取词条定义，并将相关的词条信息填充到内部数据结构中。如果初始化成功，则返回true；否则返回false。
	 *
	 * @param InEntryQuote 用于初始化词条逻辑对象的词条引用数据。
	 * @return 如果成功初始化了词条逻辑对象，则返回true；否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="Entry", DisplayName="初始化词条逻辑")
	bool InitItemEntryLogic(const FLxItemEntryQuote& InEntryQuote);

	/**
	 * @brief生成词条的显示名称文本。
	 *
	 * 该函数根据词条逻辑对象内部存储的数据，构建并返回一个表示词条显示名称的富文本。显示名称通常用于用户界面中展示给玩家看的词条标题或主要描述。
	 *
	 * @return 返回一个FText对象，包含了词条的显示名称文本。
	 */
	UFUNCTION(BlueprintPure, Category="Entry", DisplayName="生成词条的显示名称文本")
	FText BuildEntryDisplayNameText() const;

	/**
	 * @brief生成词条的显示文本。
	 *
	 * 该函数根据词条逻辑对象内部存储的数据，构建并返回一个表示词条详细信息的富文本。此文本通常用于用户界面中展示给玩家看的词条具体内容，包括词条名称及其数值等。
	 *
	 * @return 返回一个FText对象，包含了词条的显示文本。
	 */
	UFUNCTION(BlueprintPure, Category="Entry", DisplayName="生成词条的显示文本")
	FText BuildEntryDisplayText();

	/**
	 * @brief 获取词条数据。
	 *
	 * 该函数返回当前词条逻辑对象中存储的词条数据。词条数据包含了关于词条的基础信息、可视化描述信息以及各种属性和状态相关的详细数据。
	 *
	 * @return 返回一个常量引用，指向当前词条逻辑对象内部存储的FLxItemEntryData结构体。
	 */
	const FLxItemEntryData& GetItemEntryData() const { return m_ItemEntryData; }

	/**
	 * @brief 获取可修改的词条数据引用。
	 *
	 * 该函数提供对当前词条逻辑对象内部存储的词条数据的可修改引用。这允许外部代码直接访问并修改词条数据，如基础信息、可视化描述信息以及各种属性和状态相关的详细数据。
	 *
	 * @return 返回一个引用，指向当前词条逻辑对象内部存储的FLxItemEntryData结构体，允许对其进行修改。
	 */
	FLxItemEntryData& GetMutableItemEntryData() { return m_ItemEntryData; }

	/**
	 * @brief 检查词条逻辑对象是否有效。
	 *
	 * 该函数用于验证当前的ULxItemEntryLogic实例是否包含有效的词条数据。具体来说，它检查内部存储的词条基础信息中的词条ID是否为None。如果词条ID不为None，则认为该词条逻辑对象是有效的。
	 *
	 * @return 如果词条逻辑对象包含有效的词条数据（即词条ID不为None），则返回true；否则返回false。
	 */
	UFUNCTION(BlueprintPure, Category="Entry", DisplayName="检查词条逻辑对象是否有效")
	bool IsEntryValid() const;

	/**
	 * @brief 获取词条的有效数值。
	 *
	 * 该函数计算并返回当前词条逻辑对象所表示的词条的有效数值。如果存在临时有效值，则直接返回该临时值；否则，根据词条的逻辑类型（如缓冲进入、属性值变更等）和相关系数来计算最终的有效数值。
	 *
	 * @return 返回一个浮点数，代表词条的有效数值。
	 */
	UFUNCTION(BlueprintPure, Category="Entry", DisplayName="获取词条最终加成数值")
	float GetEffectiveValue() const;

	UFUNCTION(BlueprintPure, Category="Entry", DisplayName="获取加成方式")
	ELxItemEntryType GetEntryValueType() const;

	/**
	 * @brief 设置词条的临时有效数值。
	 *
	 * 该函数用于为当前词条逻辑对象设置一个临时的有效数值。这个临时值会在某些特定情况下覆盖词条原本计算出的有效数值，例如在应用了临时增益或减益效果时。一旦设置了临时有效数值，`m_bHasTemporaryEffectiveValue`标志将被设置为true，表示当前存在有效的临时数值。
	 *
	 * @param InEffectiveValue 要设置给词条逻辑对象的新临时有效数值。
	 */
	void SetTemporaryEffectiveValue(float InEffectiveValue);
	/**
	 * @brief 清除词条逻辑对象的临时有效数值。
	 *
	 * 该函数用于清除当前ULxItemEntryLogic实例中设置的任何临时有效数值。调用此函数后，`m_TemporaryEffectiveValue`将被重置为0.0f，并且`m_bHasTemporaryEffectiveValue`标志将被设置为false，表示不再存在有效的临时数值。这通常在临时增益或减益效果结束后调用，以恢复词条逻辑对象到其原始状态。
	 */
	void ClearTemporaryEffectiveValue();

	/**
	 * @brief生成富文本描述。
	 *
	 * 该函数根据提供的富文本描述组数据和参数数组，构建并返回一个表示词条详细信息的富文本。此文本通常用于用户界面中展示给玩家看的词条具体内容，包括词条名称及其数值等。
	 *
	 * @param InDescriptionGroupData 富文本描述组数据，包含主文本体和片段列表。
	 * @param InArgs 参数数组，用于替换富文本中的占位符。
	 * @return 返回一个FText对象，包含了构建好的富文本描述。
	 */
	static FText BuildRichTextDescriptionText(const FLxRichTextDescriptionGroupData& InDescriptionGroupData, const TArray<FText>& InArgs = TArray<FText>());
private:
	/**
	 * @brief 根据词条数据和最终数值构建显示文本。
	 *
	 * 该函数根据传入的词条数据和最终计算得到的数值，生成一个用于界面展示的文本。根据不同的词条类型，返回的文本格式会有所不同。
	 *
	 * @param InEntryData 词条数据结构体，包含了词条的基础信息以及逻辑类型等。
	 * @param InFinalValue 最终计算得到的数值，用于在显示文本中体现词条的具体效果。
	 * @return 返回一个FText对象，表示根据给定参数格式化后的词条显示文本。
	 */
	static FText BuildEntryValueText(const FLxItemEntryData& InEntryData, float InFinalValue);
	/**
	 * @brief 构建富文本片段字符串。
	 *
	 * 该方法根据提供的描述数据生成包含图标和样式的富文本字符串。如果提供了图标标签或文本样式标签，则会在输出的富文本字符串中相应地插入这些标签。
	 *
	 * @param InDescriptionData 描述数据，包括图标标签、文本样式标签以及要显示的文本内容。
	 * @return 返回构建好的富文本字符串。
	 */
	static FString BuildRichTextFragmentString(const FLxRichTextDescriptionData& InDescriptionData);

	/**
	 * @brief词条数据结构，用于存储和表示游戏中的词条信息。
	 *
	 * `m_ItemEntryData` 包含了与游戏内特定词条相关的所有必要信息，如词条的类型、数值、以及可能影响游戏机制的其他属性。此变量通常用于装备、角色技能或其他游戏元素上，以提供具体的加成或效果描述。通过蓝图或编辑器可直接访问并修改该词条数据，便于游戏开发过程中对词条进行定制化调整。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Entry", meta=(AllowPrivateAccess="true"))
	FLxItemEntryData m_ItemEntryData;


	/**
	 * @brief 标志，指示对象是否具有临时有效值。
	 *
	 * 此布尔变量用于跟踪当前对象或实体是否正在使用临时有效值。
	 * 当设置为true时，表示存在一个对通常值的临时覆盖，这可能是由于增益、减益或其他临时游戏机制导致的。
	 * 此标志对于确定在游戏或应用程序中的计算和显示中使用的正确行为和值至关重要。
	 */
	bool m_bHasTemporaryEffectiveValue = false;
	/**
	 * @brief临时生效值，用于存储在特定时间段内生效的数值。
	 *
	 * 该变量主要用于在游戏逻辑中表示一个临时性的数值调整，例如技能效果、状态增益或减益等。当相关事件结束时，此值通常会被重置为默认值（0.0）。通过使用 `m_TemporaryEffectiveValue`，可以灵活地实现对角色属性或其他游戏元素的短期影响，而不永久改变其基础值。
	 */
	float m_TemporaryEffectiveValue = 0.f;
};
