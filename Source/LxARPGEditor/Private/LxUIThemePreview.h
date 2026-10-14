#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;
class UUserWidget;

/** 为统一界面风格提供不依赖桌面窗口的设计器布局预览。 */
namespace LxUITheme
{
	/** 输出中文PNG；默认仅预览设计器，传入运行数据回调与测试世界时执行真实控件构造及数据绑定。 */
	bool RenderPreview(UWidgetBlueprint* Blueprint, const FString& Directory, bool bSelectedItemGrid = false,
		TFunction<void(UUserWidget*)> PrepareRuntimeData = {}, UObject* RuntimeOuter = nullptr);
}
