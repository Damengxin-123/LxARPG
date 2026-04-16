#include "LxRichTextImageDecorator.h"

#include "Components/RichTextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/TextLayout.h"
#include "Misc/DefaultValueHelper.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	ESlateBrushDrawType::Type ToBrushDrawType(ELxRichInlineImageDrawMode InDrawMode)
	{
		switch (InDrawMode)
		{
		case ELxRichInlineImageDrawMode::Box:
			return ESlateBrushDrawType::Box;

		case ELxRichInlineImageDrawMode::Border:
			return ESlateBrushDrawType::Border;

		case ELxRichInlineImageDrawMode::Image:
		default:
			return ESlateBrushDrawType::Image;
		}
	}

	ESlateBrushTileType::Type ToBrushTileType(ELxRichInlineImageTileMode InTileMode)
	{
		switch (InTileMode)
		{
		case ELxRichInlineImageTileMode::Horizontal:
			return ESlateBrushTileType::Horizontal;

		case ELxRichInlineImageTileMode::Vertical:
			return ESlateBrushTileType::Vertical;

		case ELxRichInlineImageTileMode::Both:
			return ESlateBrushTileType::Both;

		case ELxRichInlineImageTileMode::NoTile:
		default:
			return ESlateBrushTileType::NoTile;
		}
	}

	EVerticalAlignment ToVerticalAlignment(ELxRichInlineImageVerticalAlignment InAlignment)
	{
		switch (InAlignment)
		{
		case ELxRichInlineImageVerticalAlignment::Top:
			return VAlign_Top;

		case ELxRichInlineImageVerticalAlignment::Bottom:
			return VAlign_Bottom;

		case ELxRichInlineImageVerticalAlignment::Center:
		default:
			return VAlign_Center;
		}
	}

	TOptional<int32> ParseOptionalDimension(const FString* InDimensionString, const FSlateBrush& InBrush, bool bWidth)
	{
		if (InDimensionString == nullptr)
		{
			return TOptional<int32>();
		}

		int32 ParsedValue = 0;
		if (FDefaultValueHelper::ParseInt(*InDimensionString, ParsedValue))
		{
			return ParsedValue;
		}

		if (InDimensionString->Equals(TEXT("desired"), ESearchCase::IgnoreCase))
		{
			return FMath::RoundToInt(bWidth ? InBrush.ImageSize.X : InBrush.ImageSize.Y);
		}

		return TOptional<int32>();
	}

	ELxRichInlineImageVerticalAlignment ParseVerticalAlignment(const TMap<FString, FString>& MetaData, ELxRichInlineImageVerticalAlignment InDefaultValue)
	{
		const FString* AlignmentString = MetaData.Find(TEXT("align"));
		if (AlignmentString == nullptr)
		{
			AlignmentString = MetaData.Find(TEXT("valign"));
		}

		if (AlignmentString == nullptr)
		{
			return InDefaultValue;
		}

		if (AlignmentString->Equals(TEXT("top"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageVerticalAlignment::Top;
		}

		if (AlignmentString->Equals(TEXT("bottom"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageVerticalAlignment::Bottom;
		}

		return ELxRichInlineImageVerticalAlignment::Center;
	}

	ELxRichInlineImageDrawMode ParseDrawMode(const TMap<FString, FString>& MetaData, ELxRichInlineImageDrawMode InDefaultValue)
	{
		const FString* DrawString = MetaData.Find(TEXT("draw"));
		if (DrawString == nullptr)
		{
			DrawString = MetaData.Find(TEXT("mode"));
		}

		if (DrawString == nullptr)
		{
			return InDefaultValue;
		}

		if (DrawString->Equals(TEXT("box"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageDrawMode::Box;
		}

		if (DrawString->Equals(TEXT("border"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageDrawMode::Border;
		}

		return ELxRichInlineImageDrawMode::Image;
	}

	ELxRichInlineImageTileMode ParseTileMode(const TMap<FString, FString>& MetaData, ELxRichInlineImageTileMode InDefaultValue)
	{
		const FString* TileString = MetaData.Find(TEXT("tile"));
		if (TileString == nullptr)
		{
			TileString = MetaData.Find(TEXT("tiling"));
		}

		if (TileString == nullptr)
		{
			return InDefaultValue;
		}

		if (TileString->Equals(TEXT("horizontal"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageTileMode::Horizontal;
		}

		if (TileString->Equals(TEXT("vertical"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageTileMode::Vertical;
		}

		if (TileString->Equals(TEXT("both"), ESearchCase::IgnoreCase))
		{
			return ELxRichInlineImageTileMode::Both;
		}

		return ELxRichInlineImageTileMode::NoTile;
	}

	class SLxInlineRichImage : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLxInlineRichImage)
		{}
			SLATE_ARGUMENT(FSlateBrush, Brush)
			SLATE_ARGUMENT(FTextBlockStyle, TextStyle)
			SLATE_ARGUMENT(FVector2D, ImageSize)
			SLATE_ARGUMENT(ELxRichInlineImageVerticalAlignment, VerticalAlignment)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ImageBrush = InArgs._Brush;
			const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const float FontHeight = FontMeasure->GetMaxCharacterHeight(InArgs._TextStyle.Font, 1.0f);
			const float LineHeight = FMath::Max(FontHeight, InArgs._ImageSize.Y);

			ChildSlot
			[
				SNew(SBox)
				.WidthOverride(InArgs._ImageSize.X)
				.HeightOverride(LineHeight)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.VAlign(ToVerticalAlignment(InArgs._VerticalAlignment))
					[
						SNew(SBox)
						.WidthOverride(InArgs._ImageSize.X)
						.HeightOverride(InArgs._ImageSize.Y)
						[
							SNew(SImage)
							.Image(&ImageBrush)
						]
					]
				]
			];
		}

	private:
		FSlateBrush ImageBrush;
	};

	class FLxRichImageDecorator : public FRichTextDecorator
	{
	public:
		FLxRichImageDecorator(URichTextBlock* InOwner, const ULxRichTextImageDecorator* InDecorator)
			: FRichTextDecorator(InOwner)
			, Decorator(InDecorator)
		{
		}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
		{
			if (Decorator == nullptr || RunParseResult.Name != Decorator->TagName || !RunParseResult.MetaData.Contains(TEXT("id")))
			{
				return false;
			}

			const FTextRange& IdRange = RunParseResult.MetaData[TEXT("id")];
			const FString TagId = Text.Mid(IdRange.BeginIndex, IdRange.EndIndex - IdRange.BeginIndex);
			return Decorator->FindImageStyle(FName(*TagId)) != nullptr;
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& DefaultTextStyle) const override
		{
			const FString* ImageIdString = RunInfo.MetaData.Find(TEXT("id"));
			if (Decorator == nullptr || ImageIdString == nullptr)
			{
				return TSharedPtr<SWidget>();
			}

			const FLxRichInlineImageStyle* ImageStyle = Decorator->FindImageStyle(FName(*(*ImageIdString)));
			if (ImageStyle == nullptr)
			{
				return TSharedPtr<SWidget>();
			}

			FSlateBrush DisplayBrush = ImageStyle->Brush;
			DisplayBrush.DrawAs = ToBrushDrawType(ParseDrawMode(RunInfo.MetaData, ImageStyle->DrawMode));
			DisplayBrush.Tiling = ToBrushTileType(ParseTileMode(RunInfo.MetaData, ImageStyle->TileMode));

			const TOptional<int32> Width = ParseOptionalDimension(RunInfo.MetaData.Find(TEXT("width")), DisplayBrush, true);
			const TOptional<int32> Height = ParseOptionalDimension(RunInfo.MetaData.Find(TEXT("height")), DisplayBrush, false);

			FVector2D DisplaySize = ImageStyle->ImageSize;
			if (Width.IsSet())
			{
				DisplaySize.X = Width.GetValue();
			}
			if (Height.IsSet())
			{
				DisplaySize.Y = Height.GetValue();
			}
			if (DisplaySize.X <= 0.0f)
			{
				DisplaySize.X = DisplayBrush.ImageSize.X;
			}
			if (DisplaySize.Y <= 0.0f)
			{
				DisplaySize.Y = DisplayBrush.ImageSize.Y;
			}
			DisplayBrush.ImageSize = DisplaySize;

			return SNew(SLxInlineRichImage)
				.Brush(DisplayBrush)
				.TextStyle(DefaultTextStyle)
				.ImageSize(DisplaySize)
				.VerticalAlignment(ParseVerticalAlignment(RunInfo.MetaData, ImageStyle->VerticalAlignment));
		}

	private:
		const ULxRichTextImageDecorator* Decorator = nullptr;
	};
}

ULxRichTextImageDecorator::ULxRichTextImageDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> ULxRichTextImageDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FLxRichImageDecorator>(InOwner, this);
}

const FLxRichInlineImageStyle* ULxRichTextImageDecorator::FindImageStyle(FName InImageId) const
{
	return ImageStyleList.FindByPredicate([InImageId](const FLxRichInlineImageStyle& ImageStyle)
	{
		return ImageStyle.ImageId == InImageId;
	});
}
