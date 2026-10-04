#pragma once

class UWidgetBlueprint;

/** 仅在编辑器中把旧主菜单外观保存到蓝图控件，运行时继续只使用蓝图布局。 */
namespace LxMenuAppearance
{
	/** 恢复主界面的图片、字号、位置、背景层次及按钮高亮。 */
	bool RestoreMenu(UWidgetBlueprint* Blueprint);
	/** 为设置中的按钮引用与主界面一致的旧版图片资源。 */
	bool ApplySettingsTextures(UWidgetBlueprint* Blueprint);
}
