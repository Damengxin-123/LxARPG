#pragma once

class UWidgetBlueprint;

/** 编辑器迁移辅助：把主菜单存档选择恢复为蓝图设计器中的滚动按钮列表。 */
namespace LxMenuAppearance
{
	/** 恢复存档布局与显示事件；只保存新建的条目蓝图，主菜单由调用者统一编译保存。 */
	bool RestoreWorldList(UWidgetBlueprint* MenuBlueprint);
}
