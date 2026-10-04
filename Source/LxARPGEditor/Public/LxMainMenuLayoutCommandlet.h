#pragma once

#include "Commandlets/Commandlet.h"
#include "LxMainMenuLayoutCommandlet.generated.h"

/** 将默认菜单布局保存为可自由编辑的控件蓝图，运行时不再生成布局。 */
UCLASS(meta=(DisplayName="菜单蓝图布局工具", Category="界面|维护"))
class ULxMainMenuLayoutCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	/** Apply 创建空白布局，RestoreAppearance 还原旧图片外观，RepairFonts 修复字体；无参数只检查资产。 */
	virtual int32 Main(const FString& Params) override;

	/** 为编辑器脚本提供同一迁移入口，已有自定义布局始终保留。 */
	UFUNCTION(BlueprintCallable, Category="界面|维护", meta=(DisplayName="生成菜单蓝图布局"))
	static bool BuildMenuBlueprints();
};
