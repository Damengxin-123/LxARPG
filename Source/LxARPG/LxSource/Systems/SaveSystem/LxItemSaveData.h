#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxItemSaveData.generated.h"

class ULxItemBase;
class ULxItemSlotData;

/** 单个物品实例需要持久化的属性；静态名称、图标和对象引用不进入存档。 */
USTRUCT(BlueprintType, DisplayName="物品存档记录")
struct LXARPG_API FLxItemSaveRecord
{
	GENERATED_BODY()

	/** 用于从静态物品表重建实例的物品标签。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|物品", DisplayName="物品ID")
	FGameplayTag ItemIDTag;

	/** 该实例当前持有的物品数量。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|物品", DisplayName="物品数量")
	int32 ItemCount = 0;

	/** 当前实例的词条引用、比例、冷却时间和逻辑分类。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|物品", DisplayName="实例词条")
	TArray<FLxItemEntryConfig> Entries;
};

/** 物品所在槽位的稳定位置及其内容。 */
USTRUCT(BlueprintType, DisplayName="物品槽位存档记录")
struct LXARPG_API FLxItemSlotSaveRecord
{
	GENERATED_BODY()

	/** 槽位在所属容器中的索引，空槽位同样保存以保留排列。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|物品", DisplayName="槽位索引")
	int32 SlotIndex = INDEX_NONE;

	/** 槽位接受的物品分类，用于检查装备部位等配置是否兼容。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|物品", DisplayName="槽位标签")
	FGameplayTag SlotTag;

	/** 该槽位的物品数据；无有效物品标签表示空槽位。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|物品", DisplayName="物品记录")
	FLxItemSaveRecord Item;
};

/** 背包、装备、宝箱、商店和仓库共同使用的纯属性转换工具。 */
namespace LxItemSaveData
{
	/** 导出数量及运行时词条；空指针表示空槽，非法实例或词条信息不完整则失败并保留原输出。 */
	LXARPG_API bool CaptureItem(ULxItemBase* InItem, FLxItemSaveRecord& OutRecord);

	/** 按存档属性重建物品；无效数据或已删除的静态配置返回空指针。 */
	LXARPG_API ULxItemBase* CreateItem(UObject* InOuter, const FLxItemSaveRecord& InRecord);

	/** 按数组顺序导出全部槽位；任何槽位或物品采集失败时保留原输出并返回假。 */
	LXARPG_API bool CaptureSlots(const TArray<TObjectPtr<ULxItemSlotData>>& InSlots,
		TArray<FLxItemSlotSaveRecord>& OutRecords);

	/** 先检查并重建全部物品，再一次恢复目标槽位；不触发物品使用行为。 */
	LXARPG_API bool RestoreSlots(UObject* InOuter, const TArray<FLxItemSlotSaveRecord>& InRecords,
		const TArray<TObjectPtr<ULxItemSlotData>>& InSlots);
}
