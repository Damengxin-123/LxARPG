// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleTypes.h"
#include "LxRichTextDescriptionData.generated.h"

USTRUCT(BlueprintType, DisplayName="富文本描述片段")
struct FLxRichTextDescriptionData
{
	GENERATED_BODY()

	/** 可本地化的文本内容。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本内容")
	FText Text;

	/** 编辑期通过样式表行引用选择文本样式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本样式行引用", meta=(RowType="/Script/UMG.RichTextStyleRow"))
	FDataTableRowHandle TextStyleRow;

	/** 编辑期通过图标表行引用选择内联图片，可为空。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="图标样式行引用", meta=(RowType="RichImageRow"))
	FDataTableRowHandle IconRow;
	
	/** 运行时缓存的富文本标签名，不在蓝图中暴露。 */
	FName TextStyleTag = NAME_None;



	/** 运行时缓存的内联图片标签名，不在蓝图中暴露。 */
	FName IconTag = NAME_None;
};

USTRUCT(BlueprintType, DisplayName="富文本描述组")
struct FLxRichTextDescriptionGroupData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本描述主体")
	FLxRichTextDescriptionData TextMainBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="文本描述片段列表")
	TArray<FLxRichTextDescriptionData> TextFragmentList;
};
