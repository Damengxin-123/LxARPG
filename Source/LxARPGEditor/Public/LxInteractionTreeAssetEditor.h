#pragma once
#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class ULxInteractionTreeAsset;
class ULxInteractionTreeEdGraph;
class IDetailsView;
class SGraphEditor;
struct FPropertyChangedEvent;

/** 左侧节点详情、中部交互图、右侧功能配置的资产编辑器。 */
class FLxInteractionTreeAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	/** 打开资产并建立编辑器布局。 */
	void Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxInteractionTreeAsset* Asset);
	/** 解除撤销监听。 */
	virtual ~FLxInteractionTreeAssetEditor() override;
	/** 返回编辑器标识。 */
	virtual FName GetToolkitFName() const override { return TEXT("LxInteractionTreeEditor"); }
	/** 返回中文标题。 */
	virtual FText GetBaseToolkitName() const override { return FText::FromString(TEXT("交互树编辑器")); }
	/** 返回嵌入式标题前缀。 */
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("交互树"); }
	/** 返回标题颜色。 */
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.1f, 0.4f, 0.5f); }
	/** 注册主面板。 */
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 注销主面板。 */
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 撤销后刷新图和详情。 */
	virtual void PostUndo(bool bSuccess) override;
	/** 重做后刷新图和详情。 */
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
private:
	/** 创建三个配置区域。 */
	TSharedRef<SDockTab> SpawnMainTab(const FSpawnTabArgs& Args);
	/** 选中节点后分别刷新通用与功能详情。 */
	void OnSelectionChanged(const TSet<UObject*>& Selection);
	/** 编辑字段后更新标题、排序及校验消息。 */
	void OnPropertyChanged(const FPropertyChangedEvent& Event);
	/** 删除已选普通节点及其配置。 */
	void DeleteSelectedNodes();
	/** 返回实时中文校验提示。 */
	FText GetValidationText() const;
	/** 当前资产，由资产编辑器框架保持引用。 */
	TWeakObjectPtr<ULxInteractionTreeAsset> EditingAsset;
	/** 当前可视化图。 */
	TWeakObjectPtr<ULxInteractionTreeEdGraph> EditingGraph;
	/** 中央节点画布。 */
	TSharedPtr<SGraphEditor> GraphEditor;
	/** 左侧节点通用参数面板。 */
	TSharedPtr<IDetailsView> NodeDetails;
	/** 右侧共享功能参数面板。 */
	TSharedPtr<IDetailsView> FeatureDetails;
	/** 右侧任务等节点独立功能参数面板。 */
	TSharedPtr<IDetailsView> NodeFeatureDetails;
	/** 删除等键盘操作。 */
	TSharedPtr<FUICommandList> Commands;
	/** 只在选中任务节点时展开独立任务参数面板。 */
	bool bShowNodeFeatureDetails = false;
};
