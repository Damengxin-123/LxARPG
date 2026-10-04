#pragma once

#include "Commandlets/Commandlet.h"
#include "LxInventoryDragDropCommandlet.generated.h"

/** 检查并迁移物品列表在 UE 5.8 新增的拖放接收开关。 */
UCLASS(meta=(DisplayName="物品拖放迁移工具", Category="编辑器|物品"))
class ULxInventoryDragDropCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	/** 默认只检查；传入 Apply 时先备份，再为物品格子列表启用拖放并保存蓝图。 */
	virtual int32 Main(const FString& Params) override;
};
