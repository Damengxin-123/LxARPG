#pragma once

#include "CoreMinimal.h"
#include "LxItemSaveData.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxCharacterSaveData.generated.h"

/** 职业进度的持久化属性，职业定义类由标签重新查询。 */
USTRUCT(BlueprintType, DisplayName="职业存档记录")
struct LXARPG_API FLxProfessionSaveRecord
{
	GENERATED_BODY()

	/** 已学习职业的稳定标签。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|职业", DisplayName="职业ID")
	FGameplayTag ProfessionIDTag;

	/** 该职业当前等级。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|职业", DisplayName="职业等级")
	int32 Level = 1;

	/** 当前等级已累计的经验。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|职业", DisplayName="职业经验")
	float Experience = 0.f;

	/** 当前职业是否允许继续获得经验升级。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|职业", DisplayName="允许升级")
	bool bCanUpgrade = true;
};

/** 玩家需要保存的内容属性，容器中只保存数据而不保存运行时模块和物品对象。 */
USTRUCT(BlueprintType, DisplayName="玩家存档记录")
struct LXARPG_API FLxCharacterSaveRecord
{
	GENERATED_BODY()

	/** 玩家在当前存档中的稳定索引标签。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="玩家存档ID")
	FGameplayTag SaveID;

	/** 背包当前容量，用于恢复扩容结果。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="背包容量")
	int32 BackpackSlotCount = 0;

	/** 背包中的槽位布局与物品实例属性。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="背包记录")
	TArray<FLxItemSlotSaveRecord> BackpackSlots;

	/** 装备槽位布局与已装备物品实例属性。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="装备记录")
	TArray<FLxItemSlotSaveRecord> EquipmentSlots;

	/** 已接取、可提交和已完成任务的状态。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="任务记录")
	TArray<FLxQuestRuntimeRecord> QuestRecords;

	/** 已学习职业的等级、经验和升级权限。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="职业记录")
	TArray<FLxProfessionSaveRecord> Professions;
};
