#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxRichTextStyleTypes.h"

class UDataTable;

namespace LxRichTextStyleConfig
{
	/** 清空已加载的富文本样式映射缓存。 */
	void ClearRichTextStyleConfig();

	/** 记录当前富文本样式映射表资产。 */
	void SetRichTextStyleDataTable(UDataTable* InDataTable);

	/** 获取当前富文本样式映射表资产。 */
	UDataTable* GetRichTextStyleDataTable();

	/** 写入单条富文本样式映射配置。 */
	void SetRichTextStyleRow(FName InMappingRowName, const FLxRichTextStyleRow& InStyleRow);

	/** 获取完整的富文本样式映射配置表。 */
	const TMap<FGameplayTag, FLxRichTextStyleRow>& GetRichTextStyleRowMap();

	/** 按样式标签 ID 查询富文本样式映射配置。 */
	const FLxRichTextStyleRow* GetRichTextStyleRow(FGameplayTag InStyleIDTag);

	/** 按样式标签 ID 查询 RichTextBlock 可识别的样式行名。 */
	FName GetRichTextStyleTag(FGameplayTag InStyleIDTag);

	/** 按样式标签 ID 解析 RichTextBlock 可识别的样式行名。 */
	bool ResolveRichTextStyleTag(FGameplayTag InStyleIDTag, FName& OutTextStyleTag);
}