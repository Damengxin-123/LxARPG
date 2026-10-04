#pragma once
#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class ULxSkillFlowAsset;
class ULxSkillFlowEdGraph;
class IDetailsView;
class SGraphEditor;

/** 技能流程画布、节点参数和资产参数的最小编辑器。 */
class FLxSkillFlowAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	/** 建立图编辑器和参数面板。 */
	void Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxSkillFlowAsset* Asset);
	/** 解除撤销监听。 */
	virtual ~FLxSkillFlowAssetEditor() override;
	/** 返回编辑器内部标识。 */
	virtual FName GetToolkitFName() const override { return TEXT("LxSkillFlowEditor"); }
	/** 返回中文编辑器名称。 */
	virtual FText GetBaseToolkitName() const override { return FText::FromString(TEXT("技能流程编辑器")); }
	/** 返回嵌入模式的标题前缀。 */
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("技能流程"); }
	/** 返回编辑器标签颜色。 */
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.7f, 0.35f, 0.1f); }
	/** 注册主面板。 */
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 注销主面板。 */
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 撤销后同步数据并刷新画布。 */
	virtual void PostUndo(bool bSuccess) override;
	/** 重做与撤销使用相同刷新规则。 */
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
private:
	/** 创建画布和两个细节面板。 */
	TSharedRef<SDockTab> SpawnMainTab(const FSpawnTabArgs& Args);
	/** 将选中节点的配置显示到细节面板。 */
	void OnSelectionChanged(const TSet<UObject*>& Selection);
	/** 参数编辑完成后更新标题和运行配置。 */
	void OnPropertyChanged(const FPropertyChangedEvent& Event);
	/** 通过可撤销事务删除选中节点。 */
	void DeleteSelectedNodes();
	/** 返回当前配置校验结果。 */
	FText GetValidationText() const;
	/** 当前正在编辑的资产，由工具包持有。 */
	TWeakObjectPtr<ULxSkillFlowAsset> EditingAsset;
	/** 资产拥有的画布对象。 */
	TWeakObjectPtr<ULxSkillFlowEdGraph> EditingGraph;
	/** 技能流程画布控件。 */
	TSharedPtr<SGraphEditor> GraphEditor;
	/** 当前节点的参数面板。 */
	TSharedPtr<IDetailsView> NodeDetails;
	/** 释放方式和词条等共享参数面板。 */
	TSharedPtr<IDetailsView> AssetDetails;
	/** 删除快捷键命令。 */
	TSharedPtr<FUICommandList> Commands;
};
