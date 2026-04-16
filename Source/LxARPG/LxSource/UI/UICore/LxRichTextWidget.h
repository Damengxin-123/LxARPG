#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "LxRichTextTypes.h"
#include "LxRichTextWidget.generated.h"

class UDataTable;
class URichTextBlock;

UCLASS(Blueprintable, DisplayName="扩展富文本控件")
class LXARPG_API ULxRichTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void SynchronizeProperties() override;

	UFUNCTION(BlueprintCallable, Category="RichText")
	void SetRichText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category="RichText")
	void RefreshRichText();

	UFUNCTION(BlueprintPure, Category="RichText")
	URichTextBlock* GetRichTextBlock() const { return RichTextContent; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText", meta=(MultiLine="true"))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText", meta=(RequiredAssetDataTags="RowStructure=/Script/UMG.RichTextStyleRow"))
	TObjectPtr<UDataTable> TextStyleSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText")
	bool bOverrideDefaultTextStyle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText", meta=(EditCondition="bOverrideDefaultTextStyle"))
	FTextBlockStyle DefaultTextStyleOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText")
	bool bAutoWrapText = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText")
	float MinDesiredWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underline", meta=(ClampMin="1.0"))
	float UnderlineThickness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underline", meta=(ClampMin="0.0"))
	float UnderlinePadding = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inline Image")
	TArray<FLxRichInlineImageStyle> InlineImageStyleList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RichText")
	TArray<TSubclassOf<class URichTextBlockDecorator>> ExtraDecoratorClasses;

protected:
	void ApplyRichTextSettings();

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<URichTextBlock> RichTextContent = nullptr;
};
