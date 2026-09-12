#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxGlobalStaticDataManager.generated.h"

class ULxGameDataTablesManager;
class ULxQuestStaticDataModule;

/** 统一持有各领域静态数据模块的游戏实例级运行时管理器。 */
UCLASS(BlueprintType, DisplayName="全局静态数据管理器")
class LXARPG_API ULxGlobalStaticDataManager : public UObject
{
	GENERATED_BODY()

public:
	/** 使用现有数据表管理器配置初始化全部静态数据子模块。 */
	void Initialize(const ULxGameDataTablesManager* InDataTablesManager);

	/** 释放子模块持有的索引和已加载静态资产。 */
	void Deinitialize();

	/** 判断全局静态数据管理器是否已经完成初始化。 */
	UFUNCTION(BlueprintPure, Category="静态数据|全局", DisplayName="全局静态数据是否已初始化")
	bool IsInitialized() const { return bInitialized; }

	/** 获取管理任务系列索引和静态任务资产的子模块。 */
	UFUNCTION(BlueprintPure, Category="静态数据|任务", DisplayName="获取任务静态数据模块")
	ULxQuestStaticDataModule* GetQuestStaticDataModule() const { return QuestStaticDataModule; }

private:
	/** 任务领域静态数据子模块。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="静态数据|任务", DisplayName="任务静态数据模块")
	TObjectPtr<ULxQuestStaticDataModule> QuestStaticDataModule = nullptr;

	/** 是否已经创建并初始化全部当前支持的静态数据子模块。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="静态数据|全局",
		DisplayName="是否已经初始化", meta=(AllowPrivateAccess="true"))
	bool bInitialized = false;
};
