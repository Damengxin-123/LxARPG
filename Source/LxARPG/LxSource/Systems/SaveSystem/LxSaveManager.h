#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "LxSaveManager.generated.h"

class ULevel;
class ULxGameSaveData;
class ULxSaveComponentBase;

/** 游戏实例存档管理模块，统一读盘、按标签索引、收集对象属性并最终落盘。 */
UCLASS(BlueprintType, DisplayName="存档管理模块")
class LXARPG_API ULxSaveManager : public UObject
{
	GENERATED_BODY()

public:
	/** 设置本地存档槽；加载后不再允许切换，以免把旧缓存写入另一份存档。 */
	void Initialize(const FString& InSlotName = TEXT("LxARPG_AutoSave"), int32 InUserIndex = 0);

	/** 同步加载一次存档；首次运行创建空缓存，坏档和未知版本保留原文件。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="加载游戏存档")
	bool LoadSave();

	/** 采集所有已注册对象并统一写入本地槽位。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="保存全部存档")
	bool SaveAll();

	/** 最终只保存缓存，不访问可能已被清理的运行对象。 */
	bool SaveCachedData();

	/** 注册对象并按ID恢复；拒绝空ID、重复ID或恢复失败的对象。 */
	bool RegisterComponent(ULxSaveComponentBase* Component);

	/** 取消对象注册，保留它最后提交的缓存。 */
	void UnregisterComponent(ULxSaveComponentBase* Component);

	/** 仅采集已注册且ID未变的对象，防止重名对象覆盖存档。 */
	void CacheComponent(ULxSaveComponentBase* Component);

	/** 世界或流式关卡结束前缓存并注销其中对象，避免之后的清理覆盖快照。 */
	void CacheWorldBeforeCleanup(UWorld* World, ULevel* Level = nullptr);

	/** 获取统一缓存，组件仅用它查询和导入导出属性。 */
	ULxGameSaveData* GetSaveData() const { return SaveData; }

	/** 查询是否已经成功加载，可以用于蓝图晚绑定启动事件后的状态检查。 */
	UFUNCTION(BlueprintPure, Category="存档", DisplayName="存档是否已加载")
	bool IsSaveLoaded() const { return bLoaded; }

private:
	/** 单个已注册组件的稳定身份，注册后不跟随运行时ID变化。 */
	struct FRegisteredComponent
	{
		/** 组件弱引用，不阻止场景卸载。 */
		TWeakObjectPtr<ULxSaveComponentBase> Component;
		/** 注册时的精确标签ID。 */
		FGameplayTag SaveID;
		/** 是否位于玩家索引空间。 */
		bool bPlayer = false;
	};

	/** 已加载且跨地图保留的纯属性存档数据。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="存档", DisplayName="存档缓存")
	TObjectPtr<ULxGameSaveData> SaveData;

	/** 参与后续状态采集的存档组件。 */
	TArray<FRegisteredComponent> RegisteredComponents;

	/** 当前游戏实例使用的存档槽名称。 */
	FString SlotName = TEXT("LxARPG_AutoSave");

	/** 当前游戏实例使用的本地用户索引。 */
	int32 UserIndex = 0;

	/** 成功加载或创建空缓存后为真。 */
	bool bLoaded = false;

	/** 缓存有尚未写盘的对象状态，首次运行空世界不创建无意义文件。 */
	bool bDirty = false;
};
