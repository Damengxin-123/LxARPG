#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "TickableEditorObject.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTreeAsset.h"
#include "Widgets/Views/SListView.h"

class ULxProfessionTreeEdGraph;
class SGraphEditor;
class IDetailsView;
class SVerticalBox;

/** 提供分类职业列表、关系画布以及两个独立配置面板的资产编辑器。 */
class LXARPGEDITOR_API FLxProfessionTreeAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient, public FTickableEditorObject
{
public:
	/** 初始化并打开职业关系资产。 */
	void Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxProfessionTreeAsset* Asset);
	/** 清理图、属性和撤销监听。 */
	virtual ~FLxProfessionTreeAssetEditor() override;
	/** 返回编辑器内部标识。 */
	virtual FName GetToolkitFName() const override { return TEXT("LxProfessionTreeEditor"); }
	/** 返回编辑器名称。 */
	virtual FText GetBaseToolkitName() const override { return FText::FromString(TEXT("职业关系编辑器")); }
	/** 返回世界中心页签前缀。 */
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("职业关系"); }
	/** 返回页签颜色。 */
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.1f, 0.4f, 0.5f); }
	/** 注册编辑器主面板。 */
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 注销编辑器主面板。 */
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager) override;
	/** 撤销后刷新配置及连线。 */
	virtual void PostUndo(bool bSuccess) override;
	/** 重做后刷新配置及连线。 */
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	/** 在属性编辑回调结束后安全刷新视图。 */
	virtual void Tick(float DeltaTime) override;
	/** 提供编辑器刷新统计标识。 */
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FLxProfessionTreeAssetEditor, STATGROUP_Tickables); }
	/** 聚焦职业并显示对应等级和影响面板，供列表与预览测试使用。 */
	void SelectProfession(ULxProfessionTreeDefinition* Profession);
	/** 将全部职业节点缩放至画布可视区域。 */
	void FrameGraph();

private:
	/** 显示当前公式下的前几级与最后一次升级需求，不依赖旧的逐级经验缓存。 */
	FText GetExperiencePreviewText() const;
	/** 允许交互回归测试验证真实画布控件、选择联动和逐帧刷新稳定性。 */
	friend class FLxProfessionTreeInteractionTest;
	/** 创建列表、画布和细节面板的三栏布局。 */
	TSharedRef<SDockTab> SpawnMainTab(const FSpawnTabArgs& Args);
	/** 构建只有ID、名称和描述三列的列表行。 */
	TSharedRef<ITableRow> GenerateRow(ULxProfessionTreeDefinition* Item, const TSharedRef<STableViewBase>& Owner);
	/** 按当前分类刷新职业列表。 */
	void RefreshList();
	/** 切换管理分类。 */
	FReply ChangeCategory(ELxProfessionTreeCategory Category);
	/** 在当前分类添加新职业。 */
	FReply AddProfession();
	/** 删除列表选中职业及其画布节点、相关连线。 */
	FReply DeleteProfession();
	/** 将列表选中职业放到当前画布位置。 */
	FReply PlaceProfession();
	/** 双击列表职业后放置或定位节点。 */
	void PlaceListItem(ULxProfessionTreeDefinition* Profession);
	/** 处理列表选择变化。 */
	void OnListSelection(ULxProfessionTreeDefinition* Profession, ESelectInfo::Type SelectInfo);
	/** 处理画布节点选择变化。 */
	void OnGraphSelection(const TSet<UObject*>& Selection);
	/** 仅从画布移除选中节点，保留职业列表定义。 */
	void DeleteGraphNodes();
	/** 根据当前职业重新构建全部相关影响面板。 */
	void RefreshInfluences();
	/** 记录资产属性变化，延迟到下一个编辑器帧刷新。 */
	void OnPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	/** 记录连线或节点变化，刷新影响列表。 */
	void OnGraphChanged(const FEdGraphEditAction& Action);
	/** 被编辑的职业总资产。 */
	TWeakObjectPtr<ULxProfessionTreeAsset> EditingAsset;
	/** 当前资产所属的关系图。 */
	TWeakObjectPtr<ULxProfessionTreeEdGraph> EditingGraph;
	/** 当前选中的职业定义。 */
	TWeakObjectPtr<ULxProfessionTreeDefinition> SelectedProfession;
	/** 当前显示的职业分类。 */
	ELxProfessionTreeCategory ActiveCategory = ELxProfessionTreeCategory::Combat;
	/** 列表当前分类的职业引用。 */
	TArray<ULxProfessionTreeDefinition*> VisibleProfessions;
	/** 分类职业列表控件。 */
	TSharedPtr<SListView<ULxProfessionTreeDefinition*>> ProfessionList;
	/** 职业关系画布控件。 */
	TSharedPtr<SGraphEditor> GraphEditor;
	/** 职业本身的静态配置和等级加成面板。 */
	TSharedPtr<IDetailsView> ProfessionDetails;
	/** 始终显示的职业体系公共配置面板，与节点选中状态无关。 */
	TSharedPtr<IDetailsView> AssetDetails;
	/** 相关关系配置面板的容器。 */
	TSharedPtr<SVerticalBox> InfluenceBox;
	/** 节点删除等编辑器命令。 */
	TSharedPtr<FUICommandList> Commands;
	/** 属性变化监听句柄。 */
	FDelegateHandle PropertyHandle;
	/** 画布变化监听句柄。 */
	FDelegateHandle GraphHandle;
	/** 是否需要刷新列表和校验提示；该标记本身不得再次触发画布变化通知。 */
	bool bRefreshPending = false;
	/** 仅职业属性修改或撤销重做时请求一次画布重建，与画布变化回调分离。 */
	bool bGraphRefreshPending = false;
	/** 是否需要重建关系细节控件。 */
	bool bInfluencesPending = false;
	/** 防止列表与画布之间递归选中。 */
	bool bSelecting = false;
	/** 最近一次校验的可视化状态。 */
	FText ValidationText;
	/** 等待画布完成布局后再执行首次缩放，避免零尺寸视口导致节点过小。 */
	int32 InitialLayoutFrames = 0;
};
