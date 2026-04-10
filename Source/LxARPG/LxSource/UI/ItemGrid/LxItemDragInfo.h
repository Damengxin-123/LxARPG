#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "LxItemDragInfo.generated.h"

class ULxItemSlotData;

/**
 * @brief 物品格子拖拽操作数据
 *
 * 在拖拽物品格子时保存源格子索引、格子类型、限制子类型以及拖拽物品本身，
 * 供目标格子在接收拖放时判断交互规则。
 */
UCLASS()
class LXARPG_API ULxItemDragInfo : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/**
	 * @brief 源物品槽数据
	 *
	 * 该属性指向一个ULxItemSlotData对象，用于存储拖拽操作中源位置的物品槽信息。
	 * 在执行拖拽操作时，通过此属性可以访问到源物品槽的数据，如其中包含的物品、槽的状态等，
	 * 以便在目标位置进行相应的处理和验证。
	 */
	UPROPERTY()
	TObjectPtr<ULxItemSlotData> SourceSlot = nullptr;
};
