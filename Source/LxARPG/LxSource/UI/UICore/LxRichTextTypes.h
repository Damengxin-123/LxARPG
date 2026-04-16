#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "LxRichTextTypes.generated.h"

UENUM(BlueprintType)
enum class ELxRichInlineImageVerticalAlignment : uint8
{
	Top		UMETA(DisplayName="上对齐"),
	Center	UMETA(DisplayName="居中"),
	Bottom	UMETA(DisplayName="下对齐")
};

UENUM(BlueprintType)
enum class ELxRichInlineImageDrawMode : uint8
{
	Image	UMETA(DisplayName="普通图像"),
	Box		UMETA(DisplayName="Box"),
	Border	UMETA(DisplayName="Border")
};

UENUM(BlueprintType)
enum class ELxRichInlineImageTileMode : uint8
{
	NoTile		UMETA(DisplayName="不平铺"),
	Horizontal	UMETA(DisplayName="水平平铺"),
	Vertical	UMETA(DisplayName="垂直平铺"),
	Both		UMETA(DisplayName="双向平铺")
};

USTRUCT(BlueprintType)
struct FLxRichInlineImageStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	FName ImageId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	FSlateBrush Brush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	FVector2D ImageSize = FVector2D(16.0f, 16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	ELxRichInlineImageVerticalAlignment VerticalAlignment = ELxRichInlineImageVerticalAlignment::Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	ELxRichInlineImageDrawMode DrawMode = ELxRichInlineImageDrawMode::Image;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	ELxRichInlineImageTileMode TileMode = ELxRichInlineImageTileMode::NoTile;
};
