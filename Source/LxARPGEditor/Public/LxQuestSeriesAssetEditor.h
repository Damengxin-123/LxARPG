#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class FUICommandList;
class IToolkitHost;
class SDockTab;
class SGraphEditor;
class SLxQuestNodeTable;
class ULxQuestSeriesAsset;
class ULxQuestSeriesEdGraph;
struct FEdGraphEditAction;

/** 同时显示任务节点表和从左向右关系图的任务系列资产编辑器。 */
class FLxQuestSeriesAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	/** 创建并初始化一个任务系列资产编辑器。 */
	static TSharedRef<FLxQuestSeriesAssetEditor> CreateEditor(EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost, ULxQuestSeriesAsset* QuestSeriesAsset);

	/** 释放图变化监听和编辑器撤销监听。 */
	virtual ~FLxQuestSeriesAssetEditor() override;

	/** 返回资产编辑器内部名称。 */
	virtual FName GetToolkitFName() const override;

	/** 返回资产编辑器的中文基础名称。 */
	virtual FText GetBaseToolkitName() const override;

	/** 返回世界中心模式页签前缀。 */
	virtual FString GetWorldCentricTabPrefix() const override;

	/** 返回世界中心模式页签颜色。 */
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/** 注册任务系列编辑器的主界面页签。 */
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/** 注销任务系列编辑器的主界面页签。 */
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/** 撤销后刷新任务表和关系图。 */
	virtual void PostUndo(bool bSuccess) override;

	/** 重做后刷新任务表和关系图。 */
	virtual void PostRedo(bool bSuccess) override;

private:
	/** 完成任务系列资产编辑器的实际初始化。 */
	void InitQuestSeriesEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost,
		ULxQuestSeriesAsset* QuestSeriesAsset);

	/** 创建任务节点表与关系图所在的主界面页签。 */
	TSharedRef<SDockTab> SpawnMainTab(const FSpawnTabArgs& SpawnTabArgs);

	/** 确保任务系列拥有可序列化的编辑器关系图。 */
	void CreateEditorGraphIfNeeded();

	/** 当前系列任务节点变化后清理无效图节点并刷新界面。 */
	void HandleQuestNodesChanged();

	/** 图节点或连线变化后生成运行时任务关系。 */
	void HandleGraphChanged(const FEdGraphEditAction& GraphEditAction);

	/** 删除关系图中已选节点，但保留左侧任务静态配置。 */
	void DeleteSelectedGraphNodes();

	/** 判断关系图当前是否存在可删除节点。 */
	bool CanDeleteSelectedGraphNodes() const;

	/** 清理不再属于当前任务系列的关系图节点。 */
	void RemoveInvalidGraphNodes();

	/** 撤销、重做或外部修改后统一刷新编辑器。 */
	void RefreshEditor();

	/** 当前正在编辑的任务系列资产。 */
	TWeakObjectPtr<ULxQuestSeriesAsset> EditingAsset;

	/** 当前任务系列的编辑器关系图。 */
	TWeakObjectPtr<ULxQuestSeriesEdGraph> EditingGraph;

	/** 左侧任务节点表。 */
	TSharedPtr<SLxQuestNodeTable> QuestNodeTable;

	/** 右侧任务关系图控件。 */
	TSharedPtr<SGraphEditor> GraphEditor;

	/** 关系图删除等标准编辑命令。 */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	/** 当前关系图变化委托句柄。 */
	FDelegateHandle GraphChangedHandle;

	/** 防止清理图节点时递归处理图变化。 */
	bool bUpdatingGraph = false;
};
