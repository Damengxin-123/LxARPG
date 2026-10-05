#pragma once

#include "CoreMinimal.h"

/** 编辑期富文本主题审计与迁移入口。 */
namespace LxUITheme
{
	/** 记录富文本表的行样式；应用时只更新明确的通用文字颜色，并在首次修改前备份。 */
	bool UpdateRichTextStyles(bool bApply, FString& Report);
}
