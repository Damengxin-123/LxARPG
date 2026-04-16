#include "LxTextUI.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "LxUITextData.h"


void ULxTextUI::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	CreateText(ListItemObject);
}

void ULxTextUI::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void ULxTextUI::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
}

void ULxTextUI::CreateText(UObject* ListItemObject) const
{
	
	if (ULxUITextData* TextData = Cast<ULxUITextData>(ListItemObject))
	{
		if (FLxRichTextDescriptionGroupData* RichTextDescriptionGroupData = TextData->RichTextDescriptionGroupData)
		{
			// 拼接主体文本，标签咋写我忘了，后面需要进行修改
			FLxString OutText;
			if (!RichTextDescriptionGroupData->TextMainBody.IconTag.IsNone())
			{
				OutText << "<img id=\"" << RichTextDescriptionGroupData->TextMainBody.IconTag << "\"/>";
			}
			if (!RichTextDescriptionGroupData->TextMainBody.TextStyleTag.IsNone())
			{
				OutText << "<" << RichTextDescriptionGroupData->TextMainBody.TextStyleTag << ">";
				OutText << RichTextDescriptionGroupData->TextMainBody.Text;
				OutText << "</>";
			}
			// 拼接关键字  <词条正文>玩家的</><火焰关键字>火焰</><词条正文>亲和提高30%</>
			for (auto& Fragment : RichTextDescriptionGroupData->TextFragmentList)
			{
				FLxString FragmentText;
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
				FragmentText << "<" << RichTextDescriptionGroupData->TextMainBody.TextStyleTag << ">";
				OutText.Arg(FragmentText);
				// 为插入的标签后面的内容添加开始标签
				
			}
			OutText.Arg(TextData->ValueText);
			
			OnShowTextEvent.Broadcast(OutText.ToFText(), TextData->IsDarkColor);
		}
	}
}
