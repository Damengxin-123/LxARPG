#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;

/** 为统一界面风格提供不依赖桌面窗口的设计器布局预览。 */
namespace LxUITheme
{
	/** 将设计器布局输出为中文 PNG；选中格子模式通过临时空槽位回调显示高亮，不执行游戏初始化及构造逻辑。 */
	bool RenderPreview(UWidgetBlueprint* Blueprint, const FString& Directory, bool bSelectedItemGrid = false);
}
