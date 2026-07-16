#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "LxRichTextBlock.generated.h"

class UDataTable;

/**
 * 项目多格式文本块。
 *
 * 保留原生 RichTextBlock 的文本添加和显示方式，并在属性同步时自动应用
 * 蓝图中配置的默认文本样式集。
 */
UCLASS(BlueprintType, Blueprintable, DisplayName="项目多格式文本块")
class LXARPG_API ULxRichTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	/** 将默认文本样式集同步给原生 RichTextBlock。 */
	virtual void SynchronizeProperties() override;

	/**
	 * 当前控件默认使用的文本样式集。
	 * 数据表行结构使用 FLxRichTextStyleSetRow，并兼容原生富文本样式解析流程。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本样式", DisplayName="默认文本样式集", meta=(RequiredAssetDataTags="RowStructure=/Script/LxARPG.LxRichTextStyleSetRow"))
	TObjectPtr<UDataTable> DefaultTextStyleSet = nullptr;
};

