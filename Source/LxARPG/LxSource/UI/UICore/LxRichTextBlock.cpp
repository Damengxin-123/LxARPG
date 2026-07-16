#include "LxRichTextBlock.h"

#include "Engine/DataTable.h"

void ULxRichTextBlock::SynchronizeProperties()
{
	// 默认样式集是派生控件的统一配置入口，先传递给原生控件再同步文本等属性。
	SetTextStyleSet(DefaultTextStyleSet);

	Super::SynchronizeProperties();
}

