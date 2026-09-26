#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class ULxAIBehaviorTreeAsset;
class ULxAIBehaviorTreeEdGraph;
class ULxAIBehaviorTreeEdGraphNode;
class IDetailsView;
class SGraphEditor;
struct FPropertyChangedEvent;
enum class ELxAIBehaviorState : uint8;

/** AI控制配置编辑器，分别编辑资产级感知能力与行为节点。 */
class FLxAIBehaviorTreeAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	/** 打开资产并建立专用编辑器。 */
	void Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxAIBehaviorTreeAsset* Asset);
	/** 选中并聚焦指定状态的状态、阶段与行为组，详情停留在阶段或状态。 */
	void FocusState(ELxAIBehaviorState State);
	/** 聚焦五个固定事件入口，便于同时检查入口名称和优先级。 */
	void FocusEntries();
	/** 显示独立事件分析配置标签，供编辑器视觉检查使用。 */
	void FocusAnalysisTab();
	/** 解除撤销监听。 */
	virtual ~FLxAIBehaviorTreeAssetEditor() override;
	/** 返回编辑器标识。 */
	virtual FName GetToolkitFName() const override { return TEXT("LxAIBehaviorTreeEditor"); }
	/** 返回中文编辑器标题。 */
	virtual FText GetBaseToolkitName() const override { return FText::FromString(TEXT("AI控制配置编辑器")); }
	/** 返回嵌入式标题前缀。 */
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("AI控制配置"); }
	/** 返回标题颜色。 */
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.15f, 0.42f, 0.62f); }
	/** 注册主编辑面板。 */
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 注销主编辑面板。 */
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 撤销后重新同步图与详情。 */
	virtual void PostUndo(bool bSuccess) override;
	/** 重做后重新同步图与详情。 */
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
private:
	/** 创建提示、详情和图画布区域。 */
	TSharedRef<SDockTab> SpawnMainTab(const FSpawnTabArgs& Args);
	/** 创建始终绑定当前资产的独立感知能力面板。 */
	TSharedRef<SDockTab> SpawnPerceptionTab(const FSpawnTabArgs& Args);
	/** 创建单独的事件分析配置面板。 */
	TSharedRef<SDockTab> SpawnAnalysisTab(const FSpawnTabArgs& Args);
	/** 创建角色三档速度倍率配置面板。 */
	TSharedRef<SDockTab> SpawnMovementTab(const FSpawnTabArgs& Args);
	/** 显示运动倍率校验结果与计算顺序。 */
	FText GetMovementValidationText() const;
	/** 独立运动能力详情视图。 */
	TSharedPtr<IDetailsView> MovementDetails;
	/** 返回感知能力独立校验结果，不被未配置的行为节点遮蔽。 */
	FText GetPerceptionValidationText() const;
	/** 返回事件分析参数的独立校验结果。 */
	FText GetAnalysisValidationText() const;
	/** 选择节点时切换详情对象。 */
	void OnSelectionChanged(const TSet<UObject*>& Selection);
	/** 属性变化后同步派生序列并刷新显示。 */
	void OnPropertyChanged(const FPropertyChangedEvent& Event);
	/** 删除选中的普通节点。 */
	void DeleteSelectedNodes();
	/** 返回当前树的动态中文校验提示。 */
	FText GetValidationText() const;
	/** 当前编辑资产。 */
	TWeakObjectPtr<ULxAIBehaviorTreeAsset> EditingAsset;
	/** 当前编辑图。 */
	TWeakObjectPtr<ULxAIBehaviorTreeEdGraph> EditingGraph;
	/** 中央节点画布。 */
	TSharedPtr<SGraphEditor> GraphEditor;
	/** 当前节点配置详情。 */
	TSharedPtr<IDetailsView> NodeDetails;
	/** 角色类型共用感知配置的详情视图，不随节点选择切换。 */
	TSharedPtr<IDetailsView> PerceptionDetails;
	/** 角色类型共用事件分析配置的详情视图。 */
	TSharedPtr<IDetailsView> AnalysisDetails;
	/** 当前详情面板绑定的图节点。 */
	TWeakObjectPtr<ULxAIBehaviorTreeEdGraphNode> SelectedGraphNode;
	/** 删除等通用命令。 */
	TSharedPtr<FUICommandList> Commands;
	/** 本次打开时是否把旧开始节点替换为五事件入口。 */
	bool bMigratedLegacyGraph = false;
};
