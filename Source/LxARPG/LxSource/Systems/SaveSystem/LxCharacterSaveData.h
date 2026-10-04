#pragma once

#include "CoreMinimal.h"
#include "LxItemSaveData.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxCharacterSaveData.generated.h"

class APawn;
class USkeletalMesh;
class UMaterialInterface;

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

	/** 旧存档默认已有内容；新建角色首次进入游戏前为假，保留角色蓝图的初始配置。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="已有角色内容")
	bool bHasGameplayData = true;

	/** 角色种族是加载玩家类型的依据；旧档缺少此字段时从原角色类迁移。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="角色种族")
	ELxCharacterRaceType CharacterRace = ELxCharacterRaceType::None;

	/** 缓存种族表解析的角色类，并兼容没有种族字段的旧档；正式加载以种族表为准。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|玩家", DisplayName="角色类型")
	TSoftClassPtr<APawn> CharacterClass;

	/** 最后所在基础关卡的资产路径，不包含编辑器运行前缀。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|位置", DisplayName="所在关卡")
	FSoftObjectPath LevelPath;

	/** 角色保存时的世界位置、朝向和缩放。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|位置", DisplayName="角色变换")
	FTransform SavedTransform = FTransform::Identity;

	/** 旧档和新角色没有位置时使用关卡出生点。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|位置", DisplayName="拥有保存位置")
	bool bHasSavedTransform = false;

	/** 展示主体的骨骼网格体；预览不会生成可战斗的角色。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|外观", DisplayName="展示骨骼网格")
	TSoftObjectPtr<USkeletalMesh> PreviewMesh;

	/** 网格相对胶囊体的变换，保留不同角色的身高和朝向。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|外观", DisplayName="展示网格变换")
	FTransform PreviewMeshTransform = FTransform::Identity;

	/** 主体材质覆盖，恢复角色当前外观。 */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="存档|外观", DisplayName="展示材质")
	TArray<TSoftObjectPtr<UMaterialInterface>> PreviewMaterials;

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
