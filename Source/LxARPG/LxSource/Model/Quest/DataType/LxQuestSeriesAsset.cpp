#include "LxQuestSeriesAsset.h"

#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#endif

FPrimaryAssetId ULxQuestSeriesAsset::GetPrimaryAssetId() const
{
	return QuestSeriesId.IsValid()
		? FPrimaryAssetId(FPrimaryAssetType(TEXT("LxQuestSeries")), QuestSeriesId.GetTagName())
		: Super::GetPrimaryAssetId();
}

bool ULxQuestSeriesAsset::GetQuestNode(const FGameplayTag& QuestId, FLxQuestNodeDefinition& OutQuestNode) const
{
	OutQuestNode = FLxQuestNodeDefinition();
	if (const FLxQuestNodeDefinition* QuestNode = FindQuestNode(QuestId))
	{
		OutQuestNode = *QuestNode;
		return true;
	}

	return false;
}

bool ULxQuestSeriesAsset::ContainsQuestNode(const FGameplayTag& QuestId) const
{
	return FindQuestNode(QuestId) != nullptr;
}

bool ULxQuestSeriesAsset::IsQuestIdInSeries(const FGameplayTag& QuestId) const
{
	return QuestSeriesId.IsValid()
		&& QuestId.IsValid()
		&& QuestId != QuestSeriesId
		&& QuestId.MatchesTag(QuestSeriesId);
}

TArray<FGameplayTag> ULxQuestSeriesAsset::GetDirectSuccessorQuestIds(const FGameplayTag& QuestId) const
{
	TArray<FGameplayTag> Result;
	for (const FLxQuestNodeLink& QuestLink : QuestLinks)
	{
		if (QuestLink.FromQuestId == QuestId)
		{
			Result.AddUnique(QuestLink.ToQuestId);
		}
	}
	return Result;
}

TArray<FGameplayTag> ULxQuestSeriesAsset::GetDirectPredecessorQuestIds(const FGameplayTag& QuestId) const
{
	TArray<FGameplayTag> Result;
	for (const FLxQuestNodeLink& QuestLink : QuestLinks)
	{
		if (QuestLink.ToQuestId == QuestId)
		{
			Result.AddUnique(QuestLink.FromQuestId);
		}
	}
	return Result;
}

const FLxQuestNodeDefinition* ULxQuestSeriesAsset::FindQuestNode(const FGameplayTag& QuestId) const
{
	if (!QuestId.IsValid())
	{
		return nullptr;
	}

	return QuestNodes.FindByPredicate([&QuestId](const FLxQuestNodeDefinition& QuestNode)
	{
		return QuestNode.QuestId == QuestId;
	});
}

#if WITH_EDITOR
const FLxQuestNodeDefinition* ULxQuestSeriesAsset::FindQuestNodeByEditorId(const FGuid& EditorNodeId) const
{
	if (!EditorNodeId.IsValid())
	{
		return nullptr;
	}

	return QuestNodes.FindByPredicate([&EditorNodeId](const FLxQuestNodeDefinition& QuestNode)
	{
		return QuestNode.EditorNodeId == EditorNodeId;
	});
}

bool ULxQuestSeriesAsset::ContainsEditorNode(const FGuid& EditorNodeId) const
{
	return FindQuestNodeByEditorId(EditorNodeId) != nullptr;
}

bool ULxQuestSeriesAsset::RepairEditorData()
{
	bool bChanged = false;
	TSet<FGuid> UsedEditorNodeIds;
	TSet<FGameplayTag> UsedQuestIds;
	for (FLxQuestNodeDefinition& QuestNode : QuestNodes)
	{
		if (!QuestNode.EditorNodeId.IsValid()
			&& QuestNode.NodeId_DEPRECATED.IsValid()
			&& !UsedEditorNodeIds.Contains(QuestNode.NodeId_DEPRECATED))
		{
			QuestNode.EditorNodeId = QuestNode.NodeId_DEPRECATED;
			bChanged = true;
		}

		if (!QuestNode.EditorNodeId.IsValid() || UsedEditorNodeIds.Contains(QuestNode.EditorNodeId))
		{
			QuestNode.EditorNodeId = FGuid::NewGuid();
			bChanged = true;
		}
		UsedEditorNodeIds.Add(QuestNode.EditorNodeId);

		if (QuestNode.QuestId.IsValid())
		{
			if (!IsQuestIdInSeries(QuestNode.QuestId) || UsedQuestIds.Contains(QuestNode.QuestId))
			{
				QuestNode.QuestId = FGameplayTag();
				bChanged = true;
			}
			else
			{
				UsedQuestIds.Add(QuestNode.QuestId);
			}
		}
	}
	return bChanged;
}

void ULxQuestSeriesAsset::SetQuestSeriesId(const FGameplayTag& InQuestSeriesId)
{
	QuestSeriesId = InQuestSeriesId;
	for (FLxQuestNodeDefinition& QuestNode : QuestNodes)
	{
		if (QuestNode.QuestId.IsValid() && !IsQuestIdInSeries(QuestNode.QuestId))
		{
			QuestNode.QuestId = FGameplayTag();
		}
	}

	QuestLinks.RemoveAll([this](const FLxQuestNodeLink& QuestLink)
	{
		return !IsQuestIdInSeries(QuestLink.FromQuestId)
			|| !IsQuestIdInSeries(QuestLink.ToQuestId);
	});
}

UEdGraph* ULxQuestSeriesAsset::GetEditorGraph() const
{
#if WITH_EDITORONLY_DATA
	return EditorGraph;
#else
	return nullptr;
#endif
}

void ULxQuestSeriesAsset::SetEditorGraph(UEdGraph* InEditorGraph)
{
#if WITH_EDITORONLY_DATA
	Modify();
	EditorGraph = InEditorGraph;
#endif
}

void ULxQuestSeriesAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RepairEditorData();

	TSet<FGameplayTag> UsedQuestIds;
	for (FLxQuestNodeDefinition& QuestNode : QuestNodes)
	{
		if (QuestNode.QuestId.IsValid())
		{
			if (!IsQuestIdInSeries(QuestNode.QuestId) || UsedQuestIds.Contains(QuestNode.QuestId))
			{
				QuestNode.QuestId = FGameplayTag();
			}
			else
			{
				UsedQuestIds.Add(QuestNode.QuestId);
			}
		}
	}

	QuestLinks.RemoveAll([this](const FLxQuestNodeLink& QuestLink)
	{
		return !QuestLink.FromQuestId.IsValid()
			|| !QuestLink.ToQuestId.IsValid()
			|| QuestLink.FromQuestId == QuestLink.ToQuestId
			|| !ContainsQuestNode(QuestLink.FromQuestId)
			|| !ContainsQuestNode(QuestLink.ToQuestId);
	});
}
#endif
