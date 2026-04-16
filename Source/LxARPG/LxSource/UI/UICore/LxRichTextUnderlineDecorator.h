#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "LxRichTextUnderlineDecorator.generated.h"

UCLASS(Blueprintable, DisplayName="富文本下划线装饰器")
class LXARPG_API ULxRichTextUnderlineDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	ULxRichTextUnderlineDecorator(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underline")
	FName TagName = TEXT("u");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underline", meta=(ClampMin="1.0"))
	float UnderlineThickness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underline", meta=(ClampMin="0.0"))
	float UnderlinePadding = 1.0f;
};
