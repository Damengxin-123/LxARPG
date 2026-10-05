#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "LxSaveManager.generated.h"

class ULevel;
class ULxGameSaveData;
class ULxSaveComponentBase;

/** 正式会话采集完成后交给多存档仓库提交角色和地图快照。 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FLxPersistSaveSession, const ULxGameSaveData*);
enum class ELxSaveRecordType : uint8;

/** 游戏实例存档管理模块，统一读盘、按标签索引、收集对象属性并最终落盘。 */
UCLASS(BlueprintType, DisplayName="存档管理模块")
class LXARPG_API ULxSaveManager : public UObject
{
	GENERATED_BODY()

public:
	/** 为全新的管理器安装正式会话缓存；拒绝替换已经开始使用的缓存。 */
	bool InitializeSession(const ULxGameSaveData* InData, FLxPersistSaveSession InWriter);

	/** 在只读菜单中更换会话数据，保留已注册单位；切换地图档时原地恢复单位初始值和存档。 */
	bool ReplaceSession(const ULxGameSaveData* InData, bool bRestoreWorld);

	/** 开关菜单只读保护，禁止采集和落盘；多存档会话仍可注册并恢复背景单位。 */
	void SetReadOnly(bool bInReadOnly) { bReadOnly = bInReadOnly; }

	/** 查询当前是否为菜单只读模式。 */
	UFUNCTION(BlueprintPure, Category="存档|菜单", DisplayName="是否只读预览")
	bool IsReadOnly() const { return bReadOnly; }

	/** 正式读档中是否有对象恢复失败，用于阻止错误会话进入可保存状态。 */
	bool HasRestoreFailures() const { return bRestoreFailed; }
	/** 设置本地存档槽；加载后不再允许切换，以免把旧缓存写入另一份存档。 */
	void Initialize(const FString& InSlotName = TEXT("LxARPG_AutoSave"), int32 InUserIndex = 0);

	/** 同步加载一次存档；首次运行创建空缓存，坏档和未知版本保留原文件。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="加载游戏存档")
	bool LoadSave();

	/** 采集所有已注册对象并统一写入本地槽位；任何对象采集失败都返回失败并保留原文件。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="保存全部存档")
	bool SaveAll();

	/** 最终只保存缓存，不访问可能已被清理的运行对象；仍有采集失败时禁止落盘。 */
	bool SaveCachedData();

	/** 注册对象并按ID恢复；拒绝空ID、重复ID或恢复失败的对象。 */
	bool RegisterComponent(ULxSaveComponentBase* Component);

	/** 取消对象注册，保留它最后提交的缓存。 */
	void UnregisterComponent(ULxSaveComponentBase* Component);

	/** 仅采集已注册且ID未变的对象，成功后提交快照，失败时保留原缓存并回报失败。 */
	bool CacheComponent(ULxSaveComponentBase* Component);

	/** 世界或流式关卡结束前缓存并注销其中对象；采集失败状态在注销后仍保留。 */
	bool CacheWorldBeforeCleanup(UWorld* World, ULevel* Level = nullptr);

	/** 获取统一缓存，组件仅用它查询和导入导出属性。 */
	ULxGameSaveData* GetSaveData() const { return SaveData; }

	/** 查询是否已经成功加载，可以用于蓝图晚绑定启动事件后的状态检查。 */
	UFUNCTION(BlueprintPure, Category="存档", DisplayName="存档是否已加载")
	bool IsSaveLoaded() const { return bLoaded; }

private:
	/** 多存档会话的最终提交回调；未设置时继续使用旧单槽格式。 */
	FLxPersistSaveSession SessionWriter;
	/** 菜单阶段禁止修改运行缓存和磁盘文件。 */
	bool bReadOnly = false;
	/** 有对象恢复失败时不再自动保存部分恢复的世界。 */
	bool bRestoreFailed = false;
	/** 单个已注册组件的稳定身份，注册后不跟随运行时ID变化。 */
	struct FRegisteredComponent
	{
		/** 组件弱引用，不阻止场景卸载。 */
		TWeakObjectPtr<ULxSaveComponentBase> Component;
		/** 注册时的精确标签ID。 */
		FGameplayTag SaveID;
		/** 注册时使用的独立记录类型索引空间。 */
		ELxSaveRecordType RecordType;
	};

	/** 已加载且跨地图保留的纯属性存档数据。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="存档", DisplayName="存档缓存")
	TObjectPtr<ULxGameSaveData> SaveData;

	/** 参与后续状态采集的存档组件。 */
	TArray<FRegisteredComponent> RegisteredComponents;

	/** 尚未成功重试的采集失败对象；弱引用失效或注销也不能把旧快照当作最新进度保存。 */
	TSet<TWeakObjectPtr<ULxSaveComponentBase>> FailedCaptureComponents;

	/** 当前游戏实例使用的存档槽名称。 */
	FString SlotName = TEXT("LxARPG_AutoSave");

	/** 当前游戏实例使用的本地用户索引。 */
	int32 UserIndex = 0;

	/** 成功加载或创建空缓存后为真。 */
	bool bLoaded = false;

	/** 缓存有尚未写盘的对象状态，首次运行空世界不创建无意义文件。 */
	bool bDirty = false;
};
