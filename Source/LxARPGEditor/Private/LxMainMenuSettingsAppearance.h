#pragma once

class UWidgetBlueprint;

/** 编辑器内恢复主菜单控件蓝图的旧版外观。 */
namespace LxMenuAppearance
{

/** 恢复设置页的文字、间距和标准控件样式，保留现有控件及蓝图事件，由调用方统一编译保存。 */
bool RestoreSettings(UWidgetBlueprint* Blueprint);

}
