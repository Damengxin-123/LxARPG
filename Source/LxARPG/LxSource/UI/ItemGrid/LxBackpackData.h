/**
 * @file LxBackpackData.h
 * @brief 背包与格子显示用的数据对象
 */

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "UObject/Object.h"
#include "LxBackpackData.generated.h"

class ULxItemData;
class ULxCharacterBackpackComponent;

/**
 * @brief TileView 物品格子数据
 *
 * 由背包、装备、快捷栏等容器创建后传给 TileView，
 * 每个对象对应一个格子需要显示和交互的数据。
 */
UCLASS(BlueprintType, DisplayName="UI数据|物品格子数据")
class LXARPG_API ULxBackpackData : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化格子数据。 */
	UFUNCTION(BlueprintCallable, Category="Item Grid")
	void InitializeGridData(int32 InIndex, ULxItemData* InItemData, EItemSlotWidgetType InSlotType, int32 InItemSubType = 0, ULxCharacterBackpackComponent* InBackpackComponent = nullptr);

	/** 当前格子在所属容器中的索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品格子数据|格子索引")
	int32 m_nIndex = INDEX_NONE;

	/** 当前格子关联的物品数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品格子数据|物品数据")
	TObjectPtr<ULxItemData> m_pItemData = nullptr;

	/** 当前格子的功能类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品格子数据|格子类型")
	EItemSlotWidgetType m_nItemType = EItemSlotWidgetType::EIT_None;

	/** 当前格子的限制子类型，例如装备位类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品格子数据|限制子类型")
	int32 m_nItemSubType = INDEX_NONE;

	/** 背包组件引用，主要用于背包来源格子的额外逻辑。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品格子数据|背包组件")
	TObjectPtr<ULxCharacterBackpackComponent> m_pBackpackComponent = nullptr;
};
