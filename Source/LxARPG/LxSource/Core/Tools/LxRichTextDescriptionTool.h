#pragma once

#include "CoreMinimal.h"
#include "LxString.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryEnum.h"
#include "LxARPG/LxSource/Model/Style/DataType/LxRichTextDescriptionData.h"

class LXARPG_API FLxRichTextDescriptionTool
{
public:
	static void ResolveDescriptionGroupTags(FLxRichTextDescriptionGroupData& InOutDescriptionGroupData);

	static void ResolveDescriptionTags(FLxRichTextDescriptionData& InOutDescriptionData);

	/**
	 * @brief 从提供的描述组数据生成富文本。
	 *
	 * 该函数处理给定的`FLxRichTextDescriptionGroupData`，并返回格式化的富文本字符串。输入数据在处理过程中会被读取和可能被修改。
	 *
	 * @param InOutDescriptionGroupData 要处理并用于生成富文本的富文本描述组数据的引用。
	 * @return 包含拼接后的富文本的`FLxString`。
	 */
	static FLxString MontageRichText(FLxRichTextDescriptionGroupData& InOutDescriptionGroupData);

	static FLxString ValueToString(float InValue, ELxItemEntryType InEntryType);
	
	static void UpdateRichStyleTag(FLxRichTextDescriptionData& InOutDescriptionGroupData);
};
