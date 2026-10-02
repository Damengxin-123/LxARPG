#pragma once

#include "Commandlets/Commandlet.h"
#include "LxItemTooltipCommandlet.generated.h"

/** 检查、修复和验证物品弹窗蓝图的分类显示规则。 */
UCLASS(meta=(DisplayName="物品弹窗修复工具", Category="界面|维护"))
class ULxItemTooltipCommandlet : public UCommandlet
{

	GENERATED_BODY()

public:
	/** 默认导出蓝图结构；Apply 修复资产，Verify 验证实际控件显示。 */
	virtual int32 Main(const FString& Params) override;
};
