// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxRichTextDescriptionData.generated.h"

/**
 * @brief 富文本描述片段数据
 *
 * 在使用时，如果需要为动态数值显示设置样式，可以在Text文本中添加占位符，如{0}
 * 然后将数字通过FLxString类型传入，替换占位符
 * 用于描述一段可显示在 URichTextBlock 中的文本片段，
 * 同时携带文本样式标签和图标标签名称，便于后续拼装描述文本。
 */
USTRUCT(BlueprintType, DisplayName="富文本描述片段")
struct FLxRichTextDescriptionData
{
	GENERATED_BODY()

	/** 可用于本地化/翻译的文本内容 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本内容")
	FText Text;

	/** URichTextBlock 使用的文本样式标签名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本样式")
	FName TextStyleTag = NAME_None;

	/** URichTextBlock 使用的图标标签名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="图标名称")
	FName IconTag = NAME_None;
};

/**
 * @brief 富文本数据类型
 *
 * 该结构用于组织和管理富文本描述的相关数据，包括一个主要的文本描述主体以及一组文本描述片段。
 * 主要目的是在 URichTextBlock 中显示具有多种样式和图标标签的复杂文本内容。
 *
 * 文本描述主体包含了主要的文本信息及其样式和图标标签。而文本描述片段列表则允许定义多个附加的文本片段，
 * 每个片段都可拥有独立的文本内容、样式标签和图标标签，从而支持更灵活多变的富文本文档构建。
 */
USTRUCT(BlueprintType, DisplayName="富文本数据类型")
struct FLxRichTextDescriptionGroupData
{
	GENERATED_BODY()
	/**
	 * @brief 文本描述主体
	 *
	 * 该变量存储了主要的文本描述内容，包括可本地化/翻译的文本、文本样式标签和图标标签名称。
	 * 作为 FLxRichTextDescriptionGroupData 结构的一部分，它主要用于定义在 URichTextBlock 中显示的主要文本部分。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本描述主体")
	FLxRichTextDescriptionData TextMainBody;

	/**
	 * @brief 文本描述片段列表
	 *
	 * 该变量存储了一组文本描述片段，每个片段都包含了可本地化/翻译的文本内容、文本样式标签以及图标标签名称。
	 * 这些片段可以用于在 URichTextBlock 中构建复杂的富文本文档，通过组合不同的样式和图标来增强视觉表现力。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本描述片段列表")
	TArray<FLxRichTextDescriptionData> TextFragmentList;
};
