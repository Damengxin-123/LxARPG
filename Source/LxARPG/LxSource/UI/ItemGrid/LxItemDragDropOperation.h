#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "LxItemDragDropOperation.generated.h"

class ULxItemData;
class ULxItemGridWidget;

/**
 * @brief 物品格子拖拽操作数据
 *
 * 在拖拽物品格子时保存源格子索引、格子类型、限制子类型以及拖拽物品本身，
 * 供目标格子在接收拖放时判断交互规则。
 */
UCLASS()
class LXARPG_API ULxItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** 发起拖拽的源格子索引。 */
	UPROPERTY(BlueprintReadOnly, Category="Item Grid Drag")
	int32 ItemIndex = INDEX_NONE;

	/** 发起拖拽的源格子类型。 */
	UPROPERTY(BlueprintReadOnly, Category="Item Grid Drag")
	EItemSlotWidgetType SlotWidgetType = EItemSlotWidgetType::EIT_None;

	/** 发起拖拽的源格子限制子类型。 */
	UPROPERTY(BlueprintReadOnly, Category="Item Grid Drag")
	int32 SlotSubType = INDEX_NONE;

	/** 本次拖拽携带的物品数据。 */
	UPROPERTY()
	TObjectPtr<ULxItemData> ItemData = nullptr;

	/** 发起拖拽的源格子控件。 */
	UPROPERTY()
	TObjectPtr<ULxItemGridWidget> SourceWidget = nullptr;
};
