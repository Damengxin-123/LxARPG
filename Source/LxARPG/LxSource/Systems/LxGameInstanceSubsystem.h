// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LxGameInstanceSubsystem.generated.h"

class ULxGameDataTablesManager;
class ULxGlobalStaticDataManager;
class ULxAINavigationRegistry;
class ULxSaveManager;
class ULevel;

/** 游戏生命周期发出的存档操作请求。 */
DECLARE_MULTICAST_DELEGATE(FLxSaveLifecycleRequested);
/** 加载或保存完成后回报成功状态。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FLxSaveOperationFinished, bool);
/**
 * 
 */
UCLASS(DisplayName="游戏实例全局子系统")
class LXARPG_API ULxGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/**
	 * @brief 通过世界对象获取游戏实例子系统。
	 *
	 * @param InWorldPtr 目标世界对象。
	 * @return 若世界与游戏实例有效则返回子系统实例，否则返回 nullptr。
	 */
	static ULxGameInstanceSubsystem* GetInstance(const UWorld* InWorldPtr);
	
	/**
	 * @brief 初始化游戏实例子系统。
	 *
	 * @param Collection 子系统集合，用于初始化阶段管理依赖关系。
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 释放全局静态数据管理器及其全部子模块缓存。 */
	virtual void Deinitialize() override;

	/**
	 * @brief 获取游戏数据表管理器。
	 *
	 * @return 返回当前游戏实例持有的数据表管理器对象。
	 */
	const ULxGameDataTablesManager* GetGameDataManager() const;

	/** 获取游戏实例持有的全局静态数据管理器。 */
	UFUNCTION(BlueprintPure, Category="静态数据|全局", DisplayName="获取全局静态数据管理器")
	ULxGlobalStaticDataManager* GetGlobalStaticDataManager() const;

	/** 获取游戏实例持有的 AI 导航场景对象注册表。 */
	UFUNCTION(BlueprintPure, Category="AI导航", DisplayName="获取AI导航注册表")
	ULxAINavigationRegistry* GetAINavigationRegistry() const;

	/** 获取统一持有玩家和交互对象存档的管理模块。 */
	UFUNCTION(BlueprintPure, Category="存档", DisplayName="获取存档管理模块")
	ULxSaveManager* GetSaveManager() const { return SaveManager; }

	/** 菜单结束后安装全新的会话管理器，避免复用上一档的注册对象或失败状态。 */
	void SetSessionSaveManager(ULxSaveManager* InManager);

	/** 发出加载请求并加载一次存档；后续调用不会重置当前游戏进度。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="请求加载存档")
	bool RequestLoadSave();

	/** 发出保存请求、采集当前对象并统一保存。 */
	UFUNCTION(BlueprintCallable, Category="存档", DisplayName="请求保存存档")
	bool RequestSaveGame();

	/** 游戏实例启动时发出的加载事件，早于场景对象初始化。 */
	FLxSaveLifecycleRequested OnLoadSaveRequested;

	/** 手动保存、切换地图与结束游戏时发出的保存事件。 */
	FLxSaveLifecycleRequested OnSaveGameRequested;

	/** 存档加载完成及其结果；晚绑定者可查询管理器的已加载状态。 */
	FLxSaveOperationFinished OnLoadSaveFinished;

	/** 存档保存完成及其结果。 */
	FLxSaveOperationFinished OnSaveGameFinished;
private:
	/** 过滤当前游戏实例，在世界对象开始销毁前采集并保存。 */
	void HandleWorldBeginTearDown(UWorld* World);

	/** 流式关卡卸载前缓存其中对象，后续再次加载时直接从内存恢复。 */
	void HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	/** 广播保存请求并执行存储；最终关闭阶段仅写入缓存。 */
	bool PerformSave(bool bCacheOnly);

	/** 世界拆除事件绑定句柄，游戏实例结束时解除。 */
	FDelegateHandle WorldTearDownHandle;

	/** 流式关卡卸载事件绑定句柄。 */
	FDelegateHandle LevelRemovedHandle;

	/** 防止蓝图存档事件回调再次请求相同操作造成递归。 */
	bool bSaveOperationInProgress = false;

	/** 跨地图持有全部存档记录与组件注册信息。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="存档", DisplayName="存档管理模块")
	TObjectPtr<ULxSaveManager> SaveManager;

	/** 使用现有数据表管理器配置创建全局静态数据管理器。 */
	void InitializeGlobalStaticDataManager();

	/**
	 * @brief 加载项目运行所需的数据表。
	 *
	 * 会根据游戏设置创建数据表管理器并触发表格加载。
	 */
	void LoadDataTables();
	
	// 数据表管理对象
	UPROPERTY()
	TObjectPtr<ULxGameDataTablesManager> m_vGameDataManager;

	/** 游戏实例生命周期内持有的全局静态数据管理器。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="静态数据|全局", DisplayName="全局静态数据管理器")
	TObjectPtr<ULxGlobalStaticDataManager> GlobalStaticDataManager = nullptr;

	/** 游戏实例生命周期内持有的 AI 路线与点位注册表。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="AI导航", DisplayName="AI导航注册表")
	TObjectPtr<ULxAINavigationRegistry> AINavigationRegistry = nullptr;
};
