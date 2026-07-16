#include "LxRichTextStyleTypes.h"

#include "LxRichTextStyleConfig.h"

FString FLxRichStyledText::ToRichTextString() const
{
	return ULxRichTextStyleFunctionLibrary::MakeRichTextString(*this);
}

FText FLxRichStyledText::GetStyledText() const
{
	return ULxRichTextStyleFunctionLibrary::GetStyledText(*this);
}

bool FLxRichStyledText::ResolveTextStyleTag(FName& OutTextStyleTag) const
{
	return ULxRichTextStyleFunctionLibrary::ResolveRichTextStyleTag(TextStyleIDTag, OutTextStyleTag);
}

FString ULxRichTextStyleFunctionLibrary::MakeRichTextString(const FLxRichStyledText& InStyledText)
{
	const FString TextString = InStyledText.Text.ToString();

	FName TextStyleTag = NAME_None;
	if (!InStyledText.ResolveTextStyleTag(TextStyleTag) || TextStyleTag.IsNone())
	{
		return TextString;
	}

	return FString::Printf(TEXT("<%s>%s</>"), *TextStyleTag.ToString(), *TextString);
}

FText ULxRichTextStyleFunctionLibrary::GetStyledText(const FLxRichStyledText& InStyledText)
{
	if (!InStyledText.TextStyleIDTag.IsValid())
	{
		return InStyledText.Text;
	}

	FString TextStyleTag = InStyledText.TextStyleIDTag.ToString();
	int32 LastSeparatorIndex = INDEX_NONE;
	if (TextStyleTag.FindLastChar(TEXT('.'), LastSeparatorIndex))
	{
		TextStyleTag = TextStyleTag.RightChop(LastSeparatorIndex + 1);
	}

	if (TextStyleTag.IsEmpty())
	{
		return InStyledText.Text;
	}

	return FText::FromString(FString::Printf(TEXT("<%s>%s</>"), *TextStyleTag, *InStyledText.Text.ToString()));
}

bool ULxRichTextStyleFunctionLibrary::ResolveRichTextStyleTag(FGameplayTag InStyleIDTag, FName& OutTextStyleTag)
{
	return LxRichTextStyleConfig::ResolveRichTextStyleTag(InStyleIDTag, OutTextStyleTag);
}
