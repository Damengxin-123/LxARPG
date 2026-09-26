#pragma once

#include "CoreMinimal.h"
#include "LxItemSaveData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotEnum.h"

class ULxItemSlotData;

/** 交互模块恢复存档时使用的槽位构建工具，不负责读写存档文件。 */
namespace LxInteractionSaveHelpers
{
	/** 在独立槽位中完成校验和物品重建，失败时不修改正在使用的容器。 */
	LXARPG_API bool BuildRestoredSlots(UObject* Outer, const TArray<FLxItemSlotSaveRecord>& Records,
		int32 MinimumSlotCount, ELxItemSlotType SlotType, TArray<TObjectPtr<ULxItemSlotData>>& OutSlots);
}
