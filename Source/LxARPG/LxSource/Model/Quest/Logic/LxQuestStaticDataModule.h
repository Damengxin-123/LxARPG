#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "UObject/Object.h"
#include "LxQuestStaticDataModule.generated.h"

class UDataTable;

/** 根据任务系列标签索引并按需加载任务系列静态资产的运行时模块。 */
UCLASS(BlueprintType, DisplayName="任务静态数据模块")
class LXARPG_API ULxQuestStaticDataModule : public UObject
{
	GENERATED_BODY()

public:
	/** 从任务系列索引表构建标签到软资产引用的运行时映射。 */
	void Initialize(const UDataTable* InQuestSeriesIndexTable);

	/** 清空任务系列索引以及当前模块持有的已加载资产。 */
	void Deinitialize();

	/** 判断任务系列索引表是否已经成功载入。 */
	UFUNCTION(BlueprintPure, Category="静态数据|任务", DisplayName="任务静态数据是否已初始化")
	bool IsInitialized() const { return bInitialized; }

	/** 判断索引表中是否登记了指定任务系列。 */
	UFUNCTION(BlueprintPure, Category="静态数据|任务", DisplayName="是否登记任务系列")
	bool ContainsQuestSeries(const FGameplayTag& QuestSeriesId) const;

	/** 返回索引表登记的全部任务系列ID。 */
	UFUNCTION(BlueprintPure, Category="静态数据|任务", DisplayName="获取全部任务系列ID")
	TArray<FGameplayTag> GetRegisteredQuestSeriesIds() const;

	/** 获取已经加载的任务系列，不会触发同步资产加载。 */
	UFUNCTION(BlueprintPure, Category="静态数据|任务", DisplayName="查询已加载任务系列")
	ULxQuestSeriesAsset* FindLoadedQuestSeries(const FGameplayTag& QuestSeriesId) const;

	/** 同步加载指定任务系列并缓存，首版用于先跑通静态数据查询流程。 */
	UFUNCTION(BlueprintCallable, Category="静态数据|任务", DisplayName="加载任务系列")
	ULxQuestSeriesAsset* LoadQuestSeries(const FGameplayTag& QuestSeriesId);

	/** 通过系列ID和任务ID加载并复制取得任务静态配置。 */
	UFUNCTION(BlueprintCallable, Category="静态数据|任务", DisplayName="获取任务静态配置")
	bool GetQuestNode(const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId,
		FLxQuestNodeDefinition& OutQuestNode);

	/** 释放模块对指定任务系列资产的强引用缓存。 */
	UFUNCTION(BlueprintCallable, Category="静态数据|任务", DisplayName="卸载任务系列")
	void UnloadQuestSeries(const FGameplayTag& QuestSeriesId);

private:
	/** 索引表解析得到的任务系列软资产引用。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="静态数据|任务",
		DisplayName="任务系列索引", meta=(AllowPrivateAccess="true"))
	TMap<FGameplayTag, TSoftObjectPtr<ULxQuestSeriesAsset>> QuestSeriesIndex;

	/** 已经按需加载并由模块保持存活的任务系列资产。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="静态数据|任务",
		DisplayName="已加载任务系列", meta=(AllowPrivateAccess="true"))
	TMap<FGameplayTag, TObjectPtr<ULxQuestSeriesAsset>> LoadedQuestSeries;

	/** 索引表结构有效且已经完成解析。 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="静态数据|任务",
		DisplayName="是否已经初始化", meta=(AllowPrivateAccess="true"))
	bool bInitialized = false;
};
