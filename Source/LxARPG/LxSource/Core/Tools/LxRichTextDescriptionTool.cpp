#include "LxRichTextDescriptionTool.h"

void FLxRichTextDescriptionTool::ResolveDescriptionGroupTags(FLxRichTextDescriptionGroupData& InOutDescriptionGroupData)
{
	ResolveDescriptionTags(InOutDescriptionGroupData.TextMainBody);

	for (FLxRichTextDescriptionData& Fragment : InOutDescriptionGroupData.TextFragmentList)
	{
		ResolveDescriptionTags(Fragment);
	}
}

void FLxRichTextDescriptionTool::ResolveDescriptionTags(FLxRichTextDescriptionData& InOutDescriptionData)
{
	InOutDescriptionData.TextStyleTag = InOutDescriptionData.TextStyleRow.RowName;
	InOutDescriptionData.IconTag = InOutDescriptionData.IconRow.RowName;
}

FLxString FLxRichTextDescriptionTool::MontageRichText(FLxRichTextDescriptionGroupData& InOutDescriptionGroupData)
{
	FLxString OutText;
	UpdateRichStyleTag(InOutDescriptionGroupData.TextMainBody);
	if (!InOutDescriptionGroupData.TextMainBody.IconTag.IsNone())
	{
		OutText << "<img id=\"" << InOutDescriptionGroupData.TextMainBody.IconTag << "\"/>";
	}
	if (!InOutDescriptionGroupData.TextMainBody.TextStyleTag.IsNone())
	{
		OutText << "<" << InOutDescriptionGroupData.TextMainBody.TextStyleTag << ">";
		OutText << InOutDescriptionGroupData.TextMainBody.Text;
		OutText << "</>";
	}
	// 拼接关键字  <词条正文>玩家的</><火焰关键字>火焰</><词条正文>亲和提高30%</>
	for (auto& Fragment : InOutDescriptionGroupData.TextFragmentList)
	{
		FLxString FragmentText;
		UpdateRichStyleTag(Fragment);
		// 结束前面的标签
		FragmentText << "</>";
		if (!Fragment.IconTag.IsNone())
		{
			FragmentText << "<img id=\"" << Fragment.IconTag << "\"/>";
		}
		if (!Fragment.TextStyleTag.IsNone())
		{
			FragmentText << "<" << Fragment.TextStyleTag << ">";
			FragmentText << Fragment.Text;
			FragmentText << "</>";
		}
		FragmentText << "<" << InOutDescriptionGroupData.TextMainBody.TextStyleTag << ">";
		OutText.Arg(FragmentText);
		// 为插入的标签后面的内容添加开始标签
				
	}
	return OutText;
}

FLxString FLxRichTextDescriptionTool::ValueToString(float InValue, ELxItemEntryType InEntryType)
{
	switch (InEntryType)
	{
	case ELxItemEntryType::BasicValue:
		return FLxString::DoubleToIntStr(InValue);
	case ELxItemEntryType::BasicImprove:
	case ELxItemEntryType::AdditionalImprove:
		return FLxString::DoubleToIntStr(InValue * 100) + FLxString("%");
	case ELxItemEntryType::Mechanism:
		return FLxString(InValue == 0 ? "false" : "true");
	}
	return FLxString::DoubleToIntStr(InValue);
}

void FLxRichTextDescriptionTool::UpdateRichStyleTag(FLxRichTextDescriptionData& InOutDescriptionGroupData)
{
	if (InOutDescriptionGroupData.TextStyleRow.IsNull())
	{
		InOutDescriptionGroupData.TextStyleTag = TEXT("词条正文");
	}
	else
	{
		InOutDescriptionGroupData.TextStyleTag = InOutDescriptionGroupData.TextStyleRow.RowName;
	}
}

