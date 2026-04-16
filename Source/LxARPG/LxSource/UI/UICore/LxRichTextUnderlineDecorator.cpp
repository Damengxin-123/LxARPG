#include "LxRichTextUnderlineDecorator.h"

#include "Components/RichTextBlock.h"
#include "Framework/Text/TextLayout.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	class SLxUnderlinedTextRun : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLxUnderlinedTextRun)
			: _UnderlineThickness(1.0f)
			, _UnderlinePadding(1.0f)
		{}
			SLATE_ATTRIBUTE(FText, Text)
			SLATE_ARGUMENT(FTextBlockStyle, TextStyle)
			SLATE_ATTRIBUTE(FText, ToolTipText)
			SLATE_ARGUMENT(float, UnderlineThickness)
			SLATE_ARGUMENT(float, UnderlinePadding)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			TextStyle = InArgs._TextStyle;
			const FSlateColor UnderlineColor = TextStyle.ColorAndOpacity;

			ChildSlot
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(InArgs._Text)
					.TextStyle(&TextStyle)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, InArgs._UnderlinePadding, 0.0f, 0.0f)
				[
					SNew(SBox)
					.HeightOverride(InArgs._UnderlineThickness)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(UnderlineColor)
					]
				]
			];

			SetToolTipText(InArgs._ToolTipText);
		}

	private:
		FTextBlockStyle TextStyle;
	};

	class FLxRichUnderlineDecorator : public FRichTextDecorator
	{
	public:
		FLxRichUnderlineDecorator(URichTextBlock* InOwner, const ULxRichTextUnderlineDecorator* InDecorator)
			: FRichTextDecorator(InOwner)
			, Decorator(InDecorator)
		{
		}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
		{
			return Decorator != nullptr
				&& RunParseResult.Name == Decorator->TagName
				&& RunParseResult.ContentRange.BeginIndex < RunParseResult.ContentRange.EndIndex;
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& DefaultTextStyle) const override
		{
			const FString* TooltipString = RunInfo.MetaData.Find(TEXT("tooltip"));
			const FText TooltipText = TooltipString != nullptr && !TooltipString->IsEmpty()
				? FText::FromString(*TooltipString)
				: RunInfo.Content;

			return SNew(SLxUnderlinedTextRun)
				.Text(RunInfo.Content)
				.TextStyle(DefaultTextStyle)
				.ToolTipText(TooltipText)
				.UnderlineThickness(Decorator->UnderlineThickness)
				.UnderlinePadding(Decorator->UnderlinePadding);
		}

	private:
		const ULxRichTextUnderlineDecorator* Decorator = nullptr;
	};
}

ULxRichTextUnderlineDecorator::ULxRichTextUnderlineDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> ULxRichTextUnderlineDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FLxRichUnderlineDecorator>(InOwner, this);
}
