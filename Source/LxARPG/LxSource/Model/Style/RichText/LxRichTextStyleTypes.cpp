#include "LxRichTextStyleTypes.h"

#include "LxRichTextStyleConfig.h"

FString FLxRichStyledText::ToRichTextString() const
{
	return ULxRichTextStyleFunctionLibrary::MakeRichTextString(*this);
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

bool ULxRichTextStyleFunctionLibrary::ResolveRichTextStyleTag(FGameplayTag InStyleIDTag, FName& OutTextStyleTag)
{
	return LxRichTextStyleConfig::ResolveRichTextStyleTag(InStyleIDTag, OutTextStyleTag);
}
