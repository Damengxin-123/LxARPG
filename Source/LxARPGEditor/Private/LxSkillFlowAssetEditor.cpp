#include "LxSkillFlowAssetEditor.h"
#include "LxSkillFlowEdGraph.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void FLxSkillFlowAssetEditor::Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxSkillFlowAsset* Asset)
{
	EditingAsset = Asset;
	Asset->SetFlags(RF_Transactional);
	if (!Asset->EditorGraph)
	{
		Asset->EditorGraph = NewObject<ULxSkillFlowEdGraph>(Asset, TEXT("技能流程图"), RF_Transactional);
		Asset->EditorGraph->Schema = ULxSkillFlowEdGraphSchema::StaticClass();
	}
	EditingGraph = CastChecked<ULxSkillFlowEdGraph>(Asset->EditorGraph);
	EditingGraph->EnsureEntry();
	Commands = MakeShared<FUICommandList>();
	Commands->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &FLxSkillFlowAssetEditor::DeleteSelectedNodes));
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("LxSkillFlow_Layout_v1")
		->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split(FTabManager::NewStack()->SetHideTabWell(true)->AddTab("LxSkillFlow_Main", ETabState::OpenedTab)));
	InitAssetEditor(Mode, Host, TEXT("LxSkillFlowEditor"), Layout, true, true, Asset);
	GraphEditor->ZoomToFit(false);
	for (UEdGraphNode* Raw : EditingGraph->Nodes)
		if (auto* Node = Cast<ULxSkillFlowEdGraphNode>(Raw); Node && Node->Data && Node->Data->Kind != ELxSkillFlowNodeKind::Event)
		{
			GraphEditor->SetNodeSelection(Node, true);
			break;
		}
	GEditor->RegisterForUndo(this);
}

FLxSkillFlowAssetEditor::~FLxSkillFlowAssetEditor()
{
	if (GEditor) GEditor->UnregisterForUndo(this);
}

void FLxSkillFlowAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	FAssetEditorToolkit::RegisterTabSpawners(Manager);
	Manager->RegisterTabSpawner("LxSkillFlow_Main", FOnSpawnTab::CreateSP(this, &FLxSkillFlowAssetEditor::SpawnMainTab))
		.SetDisplayName(FText::FromString(TEXT("技能流程")));
}

void FLxSkillFlowAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	Manager->UnregisterTabSpawner("LxSkillFlow_Main");
	FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}

TSharedRef<SDockTab> FLxSkillFlowAssetEditor::SpawnMainTab(const FSpawnTabArgs& Args)
{
	auto& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	NodeDetails = Properties.CreateDetailView(DetailsArgs);
	AssetDetails = Properties.CreateDetailView(DetailsArgs);
	// 编译连接和编辑图仍为可检查属性，但不混入面向策划的参数面板。
	const auto IsUserProperty = FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		const FName Name = Property.Property.GetFName();
		return Name != TEXT("Nodes") && Name != TEXT("EditorGraph") && Name != TEXT("Next")
			&& Name != TEXT("Hit") && Name != TEXT("Finished") && Name != TEXT("Id");
	});
	NodeDetails->SetIsPropertyVisibleDelegate(IsUserProperty);
	AssetDetails->SetIsPropertyVisibleDelegate(IsUserProperty);
	NodeDetails->OnFinishedChangingProperties().AddSP(this, &FLxSkillFlowAssetEditor::OnPropertyChanged);
	AssetDetails->OnFinishedChangingProperties().AddSP(this, &FLxSkillFlowAssetEditor::OnPropertyChanged);
	AssetDetails->SetObject(EditingAsset.Get());
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FLxSkillFlowAssetEditor::OnSelectionChanged);
	GraphEditor = SNew(SGraphEditor).GraphToEdit(EditingGraph.Get()).GraphEvents(Events)
		.AdditionalCommands(Commands).IsEditable(true).ShowGraphStateOverlay(false);
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[SNew(STextBlock).Text(this, &FLxSkillFlowAssetEditor::GetValidationText).AutoWrapText(true)]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.58f)[GraphEditor.ToSharedRef()]
			+ SSplitter::Slot().Value(0.27f)[NodeDetails.ToSharedRef()]
			+ SSplitter::Slot().Value(0.15f)[AssetDetails.ToSharedRef()]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[SNew(STextBlock).Text(FText::FromString(TEXT("右键添加节点 · 命中出口可反复触发 · 结束出口在整组结束时触发 · 松手仅通知维持单元 · Delete 删除 · Ctrl+Z 撤销 · Ctrl+S 保存"))).AutoWrapText(true)]
	];
}

void FLxSkillFlowAssetEditor::OnSelectionChanged(const TSet<UObject*>& Selection)
{
	UObject* Data = nullptr;
	for (UObject* Object : Selection)
		if (auto* Node = Cast<ULxSkillFlowEdGraphNode>(Object)) { Data = Node->Data; break; }
	NodeDetails->SetObject(Data);
}

void FLxSkillFlowAssetEditor::OnPropertyChanged(const FPropertyChangedEvent& Event)
{
	if (EditingGraph.IsValid()) { EditingGraph->SynchronizeAsset(); EditingGraph->NotifyGraphChanged(); }
}

void FLxSkillFlowAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditor || !EditingGraph.IsValid()) return;
	const FScopedTransaction Transaction(FText::FromString(TEXT("删除技能节点")));
	EditingGraph->Modify(); EditingAsset->Modify();
	const TSet<UObject*> Selection = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet(); NodeDetails->SetObject(nullptr);
	for (UObject* Object : Selection)
		if (auto* Node = Cast<ULxSkillFlowEdGraphNode>(Object))
		{
			Node->Modify(); EditingGraph->GetSchema()->BreakNodeLinks(*Node); Node->DestroyNode();
		}
	EditingGraph->SynchronizeAsset(); EditingGraph->NotifyGraphChanged();
}

void FLxSkillFlowAssetEditor::PostUndo(bool bSuccess)
{
	if (!EditingGraph.IsValid()) return;
	GraphEditor->ClearSelectionSet(); NodeDetails->SetObject(nullptr);
	EditingGraph->SynchronizeAsset(); EditingGraph->NotifyGraphChanged(); AssetDetails->ForceRefresh();
}

FText FLxSkillFlowAssetEditor::GetValidationText() const
{
	FText Error;
	if (EditingAsset.IsValid() && !EditingAsset->Validate(Error))
		return FText::Format(FText::FromString(TEXT("配置待完成：{0}")), Error);
	return FText::FromString(TEXT("配置有效 · 在技能物品的“技能流程”属性中指定本资产即可使用"));
}
