#pragma once

#include "Commandlets/Commandlet.h"
#include "LxUIThemeCommandlet.generated.h"

/** 将游戏内界面的设计器样式统一为主菜单主题，并保留蓝图布局与交互图。 */
UCLASS(meta=(DisplayName="界面风格统一工具", Category="界面|维护"))
class ULxUIThemeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	/** 默认导出界面清单；Apply 备份并保存主题，Preview 额外生成离屏预览。 */
	virtual int32 Main(const FString& Params) override;
};
