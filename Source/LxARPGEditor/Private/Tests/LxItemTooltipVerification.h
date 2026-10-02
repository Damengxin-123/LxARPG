#pragma once

class UWidgetBlueprint;

namespace LxItemTooltipRepair
{
/** 使用真实物品弹窗蓝图实例验证显示规则，并把结果写入项目 Saved/ItemTooltip 目录。 */
bool VerifyWidget(UWidgetBlueprint* Blueprint);
}
