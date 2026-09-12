#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class ULxQuestSeriesAsset;
struct FGameplayTag;

/** 左侧任务节点表使用的轻量列表项。 */
struct FLxQuestNodeListItem
{
	/** 当前列表项对应的编辑器内部节点ID。 */
	FGuid EditorNodeId;
};

DECLARE_DELEGATE(FLxOnQuestNodeTableChanged);

/** 在任务系列编辑器中以表格形式编辑当前资产节点的控件。 */
class SLxQuestNodeTable : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLxQuestNodeTable) {}
		SLATE_ARGUMENT(ULxQuestSeriesAsset*, QuestSeriesAsset)
		SLATE_EVENT(FLxOnQuestNodeTableChanged, OnNodesChanged)
	SLATE_END_ARGS()

	/** 构建任务节点表及其新增、删除操作。 */
	void Construct(const FArguments& InArgs);

	/** 从任务系列资产重新生成列表项。 */
	void RefreshItems();

private:
	/** 创建只显示“任务系列”标签树的单选下拉控件。 */
	TSharedRef<SWidget> CreateQuestSeriesIdPicker();

	/** 将选择的任务系列标签写入整个数据资产。 */
	void SetQuestSeriesId(const FGameplayTag& NewQuestSeriesId);

	/** 为列表中的一个任务节点创建多列表格行。 */
	TSharedRef<ITableRow> GenerateRow(TSharedPtr<FLxQuestNodeListItem> Item,
		const TSharedRef<STableViewBase>& OwnerTable);

	/** 向当前任务系列添加一个带编辑器内部ID的新节点。 */
	FReply AddQuestNode();

	/** 删除当前选中的任务节点及其全部关系。 */
	FReply DeleteSelectedQuestNode();

	/** 判断当前是否存在可删除的任务节点。 */
	bool CanDeleteSelectedQuestNode() const;

	/** 当前正在编辑的任务系列资产。 */
	TWeakObjectPtr<ULxQuestSeriesAsset> QuestSeriesAsset;

	/** 当前任务系列节点对应的列表项。 */
	TArray<TSharedPtr<FLxQuestNodeListItem>> Items;

	/** 显示任务节点多列表格的列表控件。 */
	TSharedPtr<SListView<TSharedPtr<FLxQuestNodeListItem>>> ListView;

	/** 节点数据变化后通知资产编辑器刷新关系图。 */
	FLxOnQuestNodeTableChanged OnNodesChanged;

	/** 当前打开的任务系列ID下拉控件所编辑的标签值。 */
	TSharedPtr<FGameplayTag> EditedQuestSeriesId;
};
