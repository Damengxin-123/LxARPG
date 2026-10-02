#pragma once

#include "CoreMinimal.h"
#include "LxSaveProfiles.h"
#include "LxSaveProfileStore.generated.h"

class ULxGameSaveData;

/** 管理独立角色档、地图档和双副本目录；所有写入通过目录切换后才可见。 */
UCLASS(BlueprintType, DisplayName="多存档仓库")
class LXARPG_API ULxSaveProfileStore : public UObject
{
	GENERATED_BODY()
public:
	/** 加载目录；首次使用时迁移旧单槽，旧文件始终保留。 */
	bool Initialize(const FString& InPrefix, int32 InUserIndex, const FString& LegacySlot);
	/** 获取已验证的目录，返回空表示损坏或未初始化。 */
	UFUNCTION(BlueprintPure, Category="存档|目录", DisplayName="获取存档目录")
	ULxSaveCatalog* GetCatalog() const { return Catalog; }
	/** 获取读取失败原因，可直接显示在菜单中。 */
	UFUNCTION(BlueprintPure, Category="存档|目录", DisplayName="获取存档错误")
	FString GetLastError() const { return LastError; }
	/** 只读加载一个角色，不修改最后游玩记录或任何快照。 */
	ULxCharacterProfileSave* ReadCharacter(const FGuid& ID);
	/** 组合角色与地图数据，为现有存档组件提供一次会话的独立缓存。 */
	ULxGameSaveData* ReadSession(const FGuid& CharacterID, const FGuid& WorldID);
	/** 创建初始角色档，允许同一角色类型创建多个实例。 */
	FGuid CreateCharacter(const FString& Name, const FLxCharacterSaveRecord& Record);
	/** 创建没有单位覆盖数据的新地图档，基础关卡由角色决定。 */
	FGuid CreateWorld(const FString& Name);
	/** 写出角色与地图新快照，两者都成功后再提交目录；失败保留上次完整记录。 */
	bool SaveSession(const FGuid& CharacterID, const FGuid& WorldID, const ULxGameSaveData* Session);
	/** 仅正式成功进入游戏后记录最后游玩的组合。 */
	bool MarkPlayed(const FGuid& CharacterID, const FGuid& WorldID);
private:
	/** 双目录交替提交并回读验证，成功后替换内存目录。 */
	bool CommitCatalog(ULxSaveCatalog* Next);
	/** 检查目录身份、槽位和所引用快照是否完整。 */
	bool ValidateCatalog(const ULxSaveCatalog* Data) const;
	/** 创建本仓库独占的新快照槽位。 */
	FString NewSlot(const TCHAR* Kind) const;
	/** 当前成功提交的目录。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="存档|目录", DisplayName="当前目录")
	TObjectPtr<ULxSaveCatalog> Catalog;
	/** 文件命名前缀，自动化测试使用随机值隔离真实存档。 */
	FString Prefix;
	/** 平台存档用户索引。 */
	int32 UserIndex = 0;
	/** 当前有效目录副本，下一次写另一个槽。 */
	int32 CatalogSide = 0;
	/** 最近失败原因。 */
	FString LastError;
};
