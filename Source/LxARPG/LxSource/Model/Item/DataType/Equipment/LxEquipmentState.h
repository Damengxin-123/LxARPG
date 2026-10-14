#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxEquipmentState.generated.h"

/** 普通装备的动态词条，只记录静态词条标识及未经强化的生效比例。 */
USTRUCT(BlueprintType, DisplayName="装备普通词条")
struct LXARPG_API FLxEquipmentEntry
{
    GENERATED_BODY()

    /** 从词条配置表查找效果的稳定标识。 */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="装备|词条", DisplayName="词条ID", meta=(Categories="词条"))
    FGameplayTag EntryID;

    /** 非负生效比例，1.25表示125%，允许超过100%，保留两位小数。 */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="装备|词条", DisplayName="生效比例", meta=(ClampMin="0.0", Delta="0.01"))
    float Proportion = 1.f;
};

/** 装备专用存档内容；固定词条装备的普通词条数组始终为空。 */
USTRUCT(BlueprintType, DisplayName="装备实例记录")
struct LXARPG_API FLxEquipmentSaveRecord
{
    GENERATED_BODY()

    /** 装备静态配置ID，基础词条及固定词条通过它恢复。 */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|装备", DisplayName="装备ID")
    FGameplayTag EquipmentID;

    /** 强化增益比例，0.3表示总增30%；可以超过1，保留两位小数。 */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|装备", DisplayName="强化度", meta=(ClampMin="0.0", Delta="0.01"))
    float Enhancement = 0.f;

    /** 普通装备的动态普通词条；固定装备由静态配置恢复，不重复保存。 */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|装备", DisplayName="普通词条")
    TArray<FLxEquipmentEntry> NormalEntries;
};
