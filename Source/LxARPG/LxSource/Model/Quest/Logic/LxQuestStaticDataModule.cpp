#include "LxQuestStaticDataModule.h"

#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogLxQuestStaticData, Log, All);

void ULxQuestStaticDataModule::Initialize(const UDataTable* InQuestSeriesIndexTable)
{
	Deinitialize();
	if (!InQuestSeriesIndexTable)
	{
		UE_LOG(LogLxQuestStaticData, Warning, TEXT("任务静态数据模块未配置任务系列索引表。"));
		return;
	}

	const UScriptStruct* RowStruct = InQuestSeriesIndexTable->GetRowStruct();
	if (!RowStruct || !RowStruct->IsChildOf(FLxQuestSeriesRegistryRow::StaticStruct()))
	{
		UE_LOG(LogLxQuestStaticData, Error, TEXT("任务系列索引表 %s 的行结构不是 FLxQuestSeriesRegistryRow。"),
			*GetNameSafe(InQuestSeriesIndexTable));
		return;
	}

	TArray<FLxQuestSeriesRegistryRow*> Rows;
	InQuestSeriesIndexTable->GetAllRows<FLxQuestSeriesRegistryRow>(
		TEXT("ULxQuestStaticDataModule::Initialize"), Rows);
	for (const FLxQuestSeriesRegistryRow* Row : Rows)
	{
		if (!Row || !Row->QuestSeriesId.IsValid() || Row->QuestSeriesAsset.IsNull())
		{
			UE_LOG(LogLxQuestStaticData, Warning, TEXT("任务系列索引表 %s 包含无效行，已忽略。"),
				*GetNameSafe(InQuestSeriesIndexTable));
			continue;
		}

		if (QuestSeriesIndex.Contains(Row->QuestSeriesId))
		{
			UE_LOG(LogLxQuestStaticData, Error, TEXT("任务系列索引表中存在重复ID：%s，保留首次登记。"),
				*Row->QuestSeriesId.ToString());
			continue;
		}

		QuestSeriesIndex.Add(Row->QuestSeriesId, Row->QuestSeriesAsset);
	}

	bInitialized = true;
	UE_LOG(LogLxQuestStaticData, Log, TEXT("任务静态数据模块已载入 %d 个任务系列索引。"), QuestSeriesIndex.Num());
}

void ULxQuestStaticDataModule::Deinitialize()
{
	LoadedQuestSeries.Reset();
	QuestSeriesIndex.Reset();
	bInitialized = false;
}

bool ULxQuestStaticDataModule::ContainsQuestSeries(const FGameplayTag& QuestSeriesId) const
{
	return QuestSeriesId.IsValid() && QuestSeriesIndex.Contains(QuestSeriesId);
}

TArray<FGameplayTag> ULxQuestStaticDataModule::GetRegisteredQuestSeriesIds() const
{
	TArray<FGameplayTag> Result;
	QuestSeriesIndex.GetKeys(Result);
	Result.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		return Left.ToString() < Right.ToString();
	});
	return Result;
}

ULxQuestSeriesAsset* ULxQuestStaticDataModule::FindLoadedQuestSeries(const FGameplayTag& QuestSeriesId) const
{
	const TObjectPtr<ULxQuestSeriesAsset>* FoundAsset = LoadedQuestSeries.Find(QuestSeriesId);
	return FoundAsset ? FoundAsset->Get() : nullptr;
}

ULxQuestSeriesAsset* ULxQuestStaticDataModule::LoadQuestSeries(const FGameplayTag& QuestSeriesId)
{
	if (ULxQuestSeriesAsset* LoadedAsset = FindLoadedQuestSeries(QuestSeriesId))
	{
		return LoadedAsset;
	}

	const TSoftObjectPtr<ULxQuestSeriesAsset>* SoftAsset = QuestSeriesIndex.Find(QuestSeriesId);
	if (!SoftAsset)
	{
		UE_LOG(LogLxQuestStaticData, Warning, TEXT("任务系列ID未登记：%s。"), *QuestSeriesId.ToString());
		return nullptr;
	}

	ULxQuestSeriesAsset* LoadedAsset = SoftAsset->LoadSynchronous();
	if (!LoadedAsset)
	{
		UE_LOG(LogLxQuestStaticData, Error, TEXT("任务系列资产加载失败：%s。"), *SoftAsset->ToString());
		return nullptr;
	}

	if (LoadedAsset->GetQuestSeriesId() != QuestSeriesId)
	{
		UE_LOG(LogLxQuestStaticData, Error,
			TEXT("任务系列索引ID %s 与资产 %s 内部ID %s 不一致，拒绝缓存。"),
			*QuestSeriesId.ToString(), *GetNameSafe(LoadedAsset), *LoadedAsset->GetQuestSeriesId().ToString());
		return nullptr;
	}

	LoadedQuestSeries.Add(QuestSeriesId, LoadedAsset);
	return LoadedAsset;
}

bool ULxQuestStaticDataModule::GetQuestNode(const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId,
	FLxQuestNodeDefinition& OutQuestNode)
{
	ULxQuestSeriesAsset* QuestSeriesAsset = LoadQuestSeries(QuestSeriesId);
	return QuestSeriesAsset && QuestSeriesAsset->GetQuestNode(QuestId, OutQuestNode);
}

void ULxQuestStaticDataModule::UnloadQuestSeries(const FGameplayTag& QuestSeriesId)
{
	LoadedQuestSeries.Remove(QuestSeriesId);
}
