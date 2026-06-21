#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "LxRichTextTypes.h"
#include "LxRichTextWidget.generated.h"

class UDataTable;
class URichTextBlock;

/** 扩展富文本控件，统一配置 RichTextBlock 样式表和项目内置装饰器。 */
UCLASS(Blueprintable, meta=(DisplayName="扩展富文本控件"))
class LXARPG_API ULxRichTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void SynchronizeProperties() override;

	/** 设置当前显示的富文本内容。 */
	UFUNCTION(BlueprintCallable, Category="富文本", DisplayName="设置富文本")
	void SetRichText(const FText& InText);

	/** 刷新富文本控件设置和显示内容。 */
	UFUNCTION(BlueprintCallable, Category="富文本", DisplayName="刷新富文本")
	void RefreshRichText();

	/** 获取内部 RichTextBlock 控件。 */
	UFUNCTION(BlueprintPure, Category="富文本", DisplayName="获取富文本控件")
	URichTextBlock* GetRichTextBlock() const { return RichTextContent; }

	/** 当前显示的富文本内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="文本内容", meta=(MultiLine="true"))
	FText Text;

	/** RichTextBlock 使用的原生富文本样式表，Row Struct 必须是 FRichTextStyleRow。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="文本样式表", meta=(RequiredAssetDataTags="RowStructure=/Script/UMG.RichTextStyleRow"))
	TObjectPtr<UDataTable> TextStyleSet = nullptr;

	/** 是否覆盖默认文本样式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="是否覆盖默认文本样式")
	bool bOverrideDefaultTextStyle = false;

	/** 默认文本样式覆盖配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="默认文本样式覆盖", meta=(EditCondition="bOverrideDefaultTextStyle"))
	FTextBlockStyle DefaultTextStyleOverride;

	/** 是否自动换行。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="是否自动换行")
	bool bAutoWrapText = true;

	/** 最小期望宽度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="最小期望宽度")
	float MinDesiredWidth = 0.0f;

	/** 下划线厚度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本|下划线", DisplayName="下划线厚度", meta=(ClampMin="1.0"))
	float UnderlineThickness = 1.0f;

	/** 下划线内边距。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本|下划线", DisplayName="下划线内边距", meta=(ClampMin="0.0"))
	float UnderlinePadding = 1.0f;

	/** 内联图片样式列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本|内联图片", DisplayName="内联图片样式列表")
	TArray<FLxRichInlineImageStyle> InlineImageStyleList;

	/** 额外富文本装饰器类型列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本", DisplayName="额外装饰器类型列表")
	TArray<TSubclassOf<class URichTextBlockDecorator>> ExtraDecoratorClasses;

protected:
	/** 应用当前富文本设置到内部 RichTextBlock。 */
	void ApplyRichTextSettings();

	/** 内部 RichTextBlock 控件。 */
	UPROPERTY(BlueprintReadOnly, Category="富文本", DisplayName="富文本控件", meta=(BindWidgetOptional))
	TObjectPtr<URichTextBlock> RichTextContent = nullptr;
};
