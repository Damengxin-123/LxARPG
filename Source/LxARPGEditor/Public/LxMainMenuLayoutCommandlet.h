#pragma once

#include "Commandlets/Commandlet.h"
#include "LxMainMenuLayoutCommandlet.generated.h"

/** 将默认菜单布局保存为可自由编辑的控件蓝图，运行时不再生成布局。 */
UCLASS(meta=(DisplayName="菜单蓝图布局工具", Category="界面|维护"))
class ULxMainMenuLayoutCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	/** Apply 创建空白布局，ContinueGame 原位补充继续按钮，RestoreAppearance 还原外观，RepairFonts 修复字体。 */
	virtual int32 Main(const FString& Params) override;

	/** 为编辑器脚本提供同一迁移入口，已有自定义布局始终保留。 */
	UFUNCTION(BlueprintCallable, Category="界面|维护", meta=(DisplayName="生成菜单蓝图布局"))
	static bool BuildMenuBlueprints();

	/** 在项目配置的主菜单蓝图中补充顶部继续按钮，保留已有布局、主题及事件。 */
	UFUNCTION(BlueprintCallable, Category="界面|维护", meta=(DisplayName="补充继续游戏按钮"))
	static bool EnsureContinueGameButton();

	/** 在已有设置蓝图中原位添加输入分页及中文按键行蓝图，保留其余布局和事件。 */
	UFUNCTION(BlueprintCallable, Category="界面|维护", meta=(DisplayName="补充输入设置分页"))
	static bool EnsureInputSettingsPage();

	/** 只读列出项目实际主菜单的控件、布局槽、绑定与事件，用于诊断已有资产迁移。 */
	UFUNCTION(BlueprintCallable, Category="界面|维护", meta=(DisplayName="检查主菜单蓝图结构"))
	static FString InspectMainMenuBlueprint();
};
