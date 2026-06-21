#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxRichTextStyleTypes.generated.h"

/** 富文本样式映射行，用业务样式 ID 指向 RichTextBlock 可识别的样式表行。 */
USTRUCT(BlueprintType, meta=(DisplayName="富文本样式映射行"))
struct LXARPG_API FLxRichTextStyleRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** 样式标签 ID，用于让业务配置通过 GameplayTag 引用富文本样式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本样式", DisplayName="样式标签ID", meta=(Categories="文本样式"))
	FGameplayTag StyleIDTag;

	/** RichTextBlock 样式行引用，目标数据表的 Row Struct 必须是 FRichTextStyleRow。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="富文本样式", DisplayName="样式行引用", meta=(RowType="/Script/UMG.RichTextStyleRow"))
	FDataTableRowHandle TextStyleRow;
};

/** 带样式文本，用样式 ID 和文本内容生成 RichTextBlock 可识别的显示字符串。 */
USTRUCT(BlueprintType, meta=(DisplayName="带样式文本"))
struct LXARPG_API FLxRichStyledText
{
	GENERATED_BODY()

public:
	/** 文本样式标签 ID，会在生成富文本字符串时解析为 RichTextBlock 样式行名。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="带样式文本", DisplayName="文本样式标签ID", meta=(Categories="文本样式"))
	FGameplayTag TextStyleIDTag;

	/** 需要显示的可本地化文本内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="带样式文本", DisplayName="文本内容", meta=(MultiLine="true"))
	FText Text;

	/** 将当前数据转换为 RichTextBlock 可识别的富文本字符串。 */
	FString ToRichTextString() const;

	/** 解析当前样式标签 ID 对应的富文本样式行名。 */
	bool ResolveTextStyleTag(FName& OutTextStyleTag) const;
};

/** 富文本样式工具函数库。 */
UCLASS(meta=(DisplayName="富文本样式函数库"))
class LXARPG_API ULxRichTextStyleFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 根据带样式文本数据生成 RichTextBlock 可识别的富文本字符串。 */
	UFUNCTION(BlueprintPure, Category="富文本样式", DisplayName="生成带样式富文本字符串")
	static FString MakeRichTextString(const FLxRichStyledText& InStyledText);

	/** 根据样式标签 ID 查询富文本样式行名。 */
	UFUNCTION(BlueprintPure, Category="富文本样式", DisplayName="解析富文本样式标签")
	static bool ResolveRichTextStyleTag(FGameplayTag InStyleIDTag, FName& OutTextStyleTag);
};