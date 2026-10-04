#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxAISpawnPointSaveData.generated.h"

class ALxAICharacter;

/** 刷怪点中某个实际角色类的存活数量；普通角色和首领分别按实际类计数。 */
USTRUCT(BlueprintType, DisplayName="刷怪点怪物种类数量")
struct LXARPG_API FLxAISpawnPopulationEntry
{
	GENERATED_BODY()

	/** 生成时实际选用的AI角色类，用于读档时保留普通怪物与首领的类型。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|刷怪点", DisplayName="怪物角色类")
	TSoftClassPtr<ALxAICharacter> CharacterClass;

	/** 该实际角色类在采集时仍存活的怪物个数。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|刷怪点", DisplayName="怪物数量", meta=(ClampMin="0"))
	int32 Count = 0;
};

/** 固定刷怪点的数量快照，只保存标签、实际角色类与数量，不保存实例属性或位置。 */
USTRUCT(BlueprintType, DisplayName="固定刷怪点存档记录")
struct LXARPG_API FLxAISpawnPointSaveRecord
{
	GENERATED_BODY()

	/** 所属刷怪点的稳定标签，须与存档索引及场景中的刷怪点标签相同。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|刷怪点", DisplayName="刷怪点ID")
	FGameplayTag SaveID;

	/** 各实际角色类的存活数量；空数组表示该点已经没有存活怪物。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|刷怪点", DisplayName="怪物种类与数量")
	TArray<FLxAISpawnPopulationEntry> Population;
};
