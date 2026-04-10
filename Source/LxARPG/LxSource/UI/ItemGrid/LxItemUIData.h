/**
 * @file LxBackpackData.h
 * @brief 背包与格子显示用的数据对象
 */

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxUIDataType.h"
#include "UObject/Object.h"
#include "LxItemUIData.generated.h"

class ULxItemLogicBase;
class ULxItemSlotData;

/**
 * @brief TileView 物品格子数据
 *
 * 由背包、装备、快捷栏等容器创建后传给 TileView，
 * 每个对象对应一个格子需要显示和交互的数据。
 */
UCLASS(BlueprintType, DisplayName="UI数据|物品格子数据")
class LXARPG_API ULxItemUIData : public UObject
{
	GENERATED_BODY()

public:

	/** 当前格子关联的槽位数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品格子数据|槽位数据")
	TObjectPtr<ULxItemSlotData> m_pSlotData = nullptr;

};
