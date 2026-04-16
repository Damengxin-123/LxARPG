#include "LxRichTextWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/RichTextBlock.h"
#include "Components/RichTextBlockDecorator.h"
#include "LxRichTextImageDecorator.h"
#include "LxRichTextUnderlineDecorator.h"

void ULxRichTextWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (RichTextContent == nullptr && WidgetTree != nullptr)
	{
		if (URichTextBlock* RootRichTextBlock = Cast<URichTextBlock>(WidgetTree->RootWidget))
		{
			RichTextContent = RootRichTextBlock;
		}
		else
		{
			RichTextContent = WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), TEXT("RichTextContent"));
			WidgetTree->RootWidget = RichTextContent;
		}
	}

	ApplyRichTextSettings();
}

void ULxRichTextWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyRichTextSettings();
}

void ULxRichTextWidget::SetRichText(const FText& InText)
{
	Text = InText;
	if (RichTextContent)
	{
		RichTextContent->SetText(Text);
		RichTextContent->RefreshTextLayout();
	}
}

void ULxRichTextWidget::RefreshRichText()
{
	ApplyRichTextSettings();
}

void ULxRichTextWidget::ApplyRichTextSettings()
{
	if (RichTextContent == nullptr)
	{
		return;
	}

	TArray<TSubclassOf<URichTextBlockDecorator>> DecoratorClasses;
	DecoratorClasses.AddUnique(ULxRichTextUnderlineDecorator::StaticClass());
	DecoratorClasses.AddUnique(ULxRichTextImageDecorator::StaticClass());

	for (const TSubclassOf<URichTextBlockDecorator>& DecoratorClass : ExtraDecoratorClasses)
	{
		if (*DecoratorClass != nullptr)
		{
			DecoratorClasses.AddUnique(DecoratorClass);
		}
	}

	RichTextContent->SetDecorators(DecoratorClasses);
	RichTextContent->SetTextStyleSet(TextStyleSet);
	RichTextContent->SetMinDesiredWidth(MinDesiredWidth);
	RichTextContent->SetAutoWrapText(bAutoWrapText);

	if (bOverrideDefaultTextStyle)
	{
		RichTextContent->SetDefaultTextStyle(DefaultTextStyleOverride);
	}
	else
	{
		RichTextContent->ClearAllDefaultStyleOverrides();
	}

	if (ULxRichTextUnderlineDecorator* UnderlineDecorator = Cast<ULxRichTextUnderlineDecorator>(RichTextContent->GetDecoratorByClass(ULxRichTextUnderlineDecorator::StaticClass())))
	{
		UnderlineDecorator->UnderlineThickness = UnderlineThickness;
		UnderlineDecorator->UnderlinePadding = UnderlinePadding;
	}

	if (ULxRichTextImageDecorator* ImageDecorator = Cast<ULxRichTextImageDecorator>(RichTextContent->GetDecoratorByClass(ULxRichTextImageDecorator::StaticClass())))
	{
		ImageDecorator->ImageStyleList = InlineImageStyleList;
	}

	RichTextContent->SetText(Text);
	RichTextContent->RefreshTextLayout();
}
