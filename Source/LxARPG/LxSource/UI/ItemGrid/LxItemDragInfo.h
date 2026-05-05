#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "LxItemDragInfo.generated.h"

class ULxItemSlotData;

/** 物品格子拖拽操作数据，保存本次拖拽的源槽位。 */
UCLASS()
class LXARPG_API ULxItemDragInfo : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** 源物品槽位；目标格子通过它判断是否可以交换、堆叠或放入。 */
	UPROPERTY()
	TObjectPtr<ULxItemSlotData> SourceSlot = nullptr;
};
