#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "GameplayTagContainer.h"
#include "LxRichTextStyleSetTypes.generated.h"

/**
 * 项目富文本样式集行。
 *
 * 在原生富文本样式的基础上增加稳定的业务样式标签，既可以直接作为
 * RichTextBlock 的文本样式集使用，也可以供后续的新逻辑按标签索引样式。
 */
USTRUCT(BlueprintType, DisplayName="项目富文本样式集行")
struct LXARPG_API FLxRichTextStyleSetRow : public FRichTextStyleRow
{
	GENERATED_BODY()

public:
	/** 用于在业务数据中索引当前文本样式的标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本样式", DisplayName="样式标签ID", meta=(Categories="文本样式"))
	FGameplayTag StyleIDTag;
};

