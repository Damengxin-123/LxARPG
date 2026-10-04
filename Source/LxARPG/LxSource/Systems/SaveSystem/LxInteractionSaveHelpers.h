#pragma once

#include "CoreMinimal.h"
#include "LxInteractionSaveData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotEnum.h"

class ULxItemSlotData;

/** 交互模块恢复存档时使用的槽位构建工具，不负责读写存档文件。 */
namespace LxInteractionSaveHelpers
{
	/** 仅采集预设物品的标识和数量，并保留空槽和排列；失败时不修改输出。 */
	LXARPG_API bool CaptureSlots(const TArray<TObjectPtr<ULxItemSlotData>>& Slots,
		TArray<FLxInteractionItemSaveRecord>& OutRecords);

	/** 读取新格式，或将旧槽位转换为预设物品引用，不恢复旧实例词条。 */
	LXARPG_API bool ReadItemSlots(const FLxInteractionFeatureSaveRecord& Record,
		TArray<FLxInteractionItemSaveRecord>& OutRecords);

	/** 在独立槽位中完成校验和物品重建，失败时不修改正在使用的容器。 */
	LXARPG_API bool BuildRestoredSlots(UObject* Outer, const TArray<FLxInteractionItemSaveRecord>& Records,
		int32 MinimumSlotCount, ELxItemSlotType SlotType, TArray<TObjectPtr<ULxItemSlotData>>& OutSlots);
}
