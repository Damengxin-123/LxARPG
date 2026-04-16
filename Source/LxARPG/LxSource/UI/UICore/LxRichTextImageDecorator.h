#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "LxRichTextTypes.h"
#include "LxRichTextImageDecorator.generated.h"

UCLASS(Blueprintable, DisplayName="富文本图片装饰器")
class LXARPG_API ULxRichTextImageDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	ULxRichTextImageDecorator(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	const FLxRichInlineImageStyle* FindImageStyle(FName InImageId) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	FName TagName = TEXT("img");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	TArray<FLxRichInlineImageStyle> ImageStyleList;
};
