#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;

/** 将现有控件蓝图的静态外观统一到主菜单主题，不改布局或业务逻辑。 */
namespace LxUITheme
{
	/** 修改蓝图自身控件的样式并返回发生变化的控件数；资源缺失返回 -1，不编译、不保存。 */
	int32 ApplyWidgetTheme(UWidgetBlueprint* Blueprint);

	/** 仅调柔共用物品格子的选中轮廓颜色，保留轮廓尺寸、透明填充及所有交互状态。 */
	int32 ApplyItemGridSelectionTheme(UWidgetBlueprint* Blueprint);
}
