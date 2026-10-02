#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LxCharacterSaveData.h"
#include "LxSaveProfiles.generated.h"

/** 目录中的通用存档摘要，档案 ID 与角色种类标签相互独立。 */
USTRUCT(BlueprintType, DisplayName="存档摘要")
struct LXARPG_API FLxSaveProfile
{
	GENERATED_BODY()
	/** 档案稳定身份；同类型角色也拥有不同 ID。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="档案ID")
	FGuid ID;
	/** 用户可见名称。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="存档名称")
	FString Name;
	/** 目录当前引用的完整快照槽位。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="快照槽位")
	FString Slot;
	/** 最近成功保存时间，统一使用 UTC。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="保存时间")
	FDateTime SavedAt;
	/** 最近使用的地图档，仅角色摘要使用。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="最近地图档ID")
	FGuid LastWorldID;
};

/** 全局目录仅保存索引，不加载地图单位即可列出存档。 */
UCLASS(BlueprintType, DisplayName="存档目录")
class LXARPG_API ULxSaveCatalog : public USaveGame
{
	GENERATED_BODY()
public:
	/** 目录格式版本。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="格式版本")
	int32 Version = 1;
	/** 双目录副本使用的递增提交序号。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="提交序号")
	int64 Revision = 0;
	/** 独立角色档案列表。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="角色存档")
	TArray<FLxSaveProfile> Characters;
	/** 独立地图档案列表。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="地图存档")
	TArray<FLxSaveProfile> Worlds;
	/** 最后正式进入游戏的角色。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="最后游玩角色")
	FGuid LastCharacterID;
	/** 最后正式进入游戏的地图档。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|目录", DisplayName="最后游玩地图")
	FGuid LastWorldID;
};

/** 单个角色的独立文件，地图单位不会进入该文件。 */
UCLASS(BlueprintType, DisplayName="独立角色存档")
class LXARPG_API ULxCharacterProfileSave : public USaveGame
{
	GENERATED_BODY()
public:
	/** 角色档格式版本。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|角色", DisplayName="格式版本")
	int32 Version = 1;
	/** 此文件所属档案的独立 ID。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|角色", DisplayName="角色档案ID")
	FGuid ID;
	/** 个人进度、位置和外观数据。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|角色", DisplayName="角色记录")
	FLxCharacterSaveRecord Record;
};
