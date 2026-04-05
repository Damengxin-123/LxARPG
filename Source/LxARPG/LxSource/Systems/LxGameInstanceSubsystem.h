// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LxGameInstanceSubsystem.generated.h"

class ULxGameDataTablesManager;
/**
 * 
 */
UCLASS()
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

	/**
	 * @brief 获取游戏数据表管理器。
	 *
	 * @return 返回当前游戏实例持有的数据表管理器对象。
	 */
	const ULxGameDataTablesManager* GetGameDataManager() const;
private:

	/**
	 * @brief 加载项目运行所需的数据表。
	 *
	 * 会根据游戏设置创建数据表管理器并触发表格加载。
	 */
	void LoadDataTables();
	
	// 数据表管理对象
	UPROPERTY()
	TObjectPtr<ULxGameDataTablesManager> m_vGameDataManager;
};
