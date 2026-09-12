#include "LxQuestSeriesAssetEditor.h"

#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "GraphEditor.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "LxQuestSeriesEdGraph.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/LxQuestNodeTable.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LxQuestSeriesAssetEditor"

namespace LxQuestSeriesEditorNames
{
	const FName AppIdentifier(TEXT("LxQuestSeriesEditorApp"));
	const FName MainTabId(TEXT("LxQuestSeriesEditor_Main"));
	const FName LayoutId(TEXT("LxQuestSeriesEditor_Layout_v1"));
}

TSharedRef<FLxQuestSeriesAssetEditor> FLxQuestSeriesAssetEditor::CreateEditor(EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost, ULxQuestSeriesAsset* QuestSeriesAsset)
{
	TSharedRef<FLxQuestSeriesAssetEditor> NewEditor = MakeShared<FLxQuestSeriesAssetEditor>();
	NewEditor->InitQuestSeriesEditor(Mode, InitToolkitHost, QuestSeriesAsset);
	return NewEditor;
}

FLxQuestSeriesAssetEditor::~FLxQuestSeriesAssetEditor()
{
	if (EditingGraph.IsValid() && GraphChangedHandle.IsValid())
	{
		EditingGraph->RemoveOnGraphChangedHandler(GraphChangedHandle);
	}
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

FName FLxQuestSeriesAssetEditor::GetToolkitFName() const
{
	return LxQuestSeriesEditorNames::AppIdentifier;
}

FText FLxQuestSeriesAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "任务系列编辑器");
}

FString FLxQuestSeriesAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricPrefix", "任务系列 ").ToString();
}

FLinearColor FLxQuestSeriesAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.08f, 0.32f, 0.55f, 0.5f);
}

void FLxQuestSeriesAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "任务系列编辑器"));
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	InTabManager->RegisterTabSpawner(LxQuestSeriesEditorNames::MainTabId,
		FOnSpawnTab::CreateSP(this, &FLxQuestSeriesAssetEditor::SpawnMainTab))
		.SetDisplayName(LOCTEXT("MainTabName", "任务系列"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FLxQuestSeriesAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(LxQuestSeriesEditorNames::MainTabId);
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

void FLxQuestSeriesAssetEditor::PostUndo(bool bSuccess)
{
	RefreshEditor();
}

void FLxQuestSeriesAssetEditor::PostRedo(bool bSuccess)
{
	RefreshEditor();
}

void FLxQuestSeriesAssetEditor::InitQuestSeriesEditor(EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost, ULxQuestSeriesAsset* QuestSeriesAsset)
{
	check(QuestSeriesAsset);
	EditingAsset = QuestSeriesAsset;
	QuestSeriesAsset->SetFlags(RF_Transactional);
	if (QuestSeriesAsset->RepairEditorData())
	{
		QuestSeriesAsset->MarkPackageDirty();
	}
	CreateEditorGraphIfNeeded();

	GraphEditorCommands = MakeShared<FUICommandList>();
	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FLxQuestSeriesAssetEditor::DeleteSelectedGraphNodes),
		FCanExecuteAction::CreateSP(this, &FLxQuestSeriesAssetEditor::CanDeleteSelectedGraphNodes));

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(LxQuestSeriesEditorNames::LayoutId)
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split(
				FTabManager::NewStack()
				->SetHideTabWell(true)
				->AddTab(LxQuestSeriesEditorNames::MainTabId, ETabState::OpenedTab)));

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, LxQuestSeriesEditorNames::AppIdentifier, Layout,
		bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, QuestSeriesAsset);

	if (EditingGraph.IsValid())
	{
		GraphChangedHandle = EditingGraph->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateSP(this, &FLxQuestSeriesAssetEditor::HandleGraphChanged));
	}
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

TSharedRef<SDockTab> FLxQuestSeriesAssetEditor::SpawnMainTab(const FSpawnTabArgs& SpawnTabArgs)
{
	check(SpawnTabArgs.GetTabId() == LxQuestSeriesEditorNames::MainTabId);

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.GraphToEdit(EditingGraph.Get())
		.GraphEvents(GraphEvents)
		.ShowGraphStateOverlay(false);

	QuestNodeTable = SNew(SLxQuestNodeTable)
		.QuestSeriesAsset(EditingAsset.Get())
		.OnNodesChanged(FLxOnQuestNodeTableChanged::CreateSP(this,
			&FLxQuestSeriesAssetEditor::HandleQuestNodesChanged));

	return SNew(SDockTab)
		.Label(LOCTEXT("MainTabLabel", "任务系列"))
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.43f)
			[
				SNew(SBorder)
				.Padding(4.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f, 2.0f, 2.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("QuestNodeTableTitle", "任务节点配置表"))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						QuestNodeTable.ToSharedRef()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.57f)
			[
				SNew(SBorder)
				.Padding(4.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f, 2.0f, 2.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("QuestGraphTitle", "任务关系图（左侧前置 → 右侧后续）"))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						GraphEditor.ToSharedRef()
					]
				]
			]
		];
}

void FLxQuestSeriesAssetEditor::CreateEditorGraphIfNeeded()
{
	if (!EditingAsset.IsValid())
	{
		return;
	}

	ULxQuestSeriesEdGraph* QuestGraph = Cast<ULxQuestSeriesEdGraph>(EditingAsset->GetEditorGraph());
	if (!QuestGraph)
	{
		EditingAsset->Modify();
		QuestGraph = NewObject<ULxQuestSeriesEdGraph>(EditingAsset.Get(), TEXT("任务关系图"), RF_Transactional);
		QuestGraph->Schema = ULxQuestSeriesEdGraphSchema::StaticClass();
		EditingAsset->SetEditorGraph(QuestGraph);
		EditingAsset->MarkPackageDirty();
	}

	QuestGraph->SetFlags(RF_Transactional);
	EditingGraph = QuestGraph;
}

void FLxQuestSeriesAssetEditor::HandleQuestNodesChanged()
{
	RemoveInvalidGraphNodes();
	if (EditingGraph.IsValid())
	{
		EditingGraph->SynchronizeLinksToAsset();
		EditingGraph->NotifyGraphChanged();
	}
}

void FLxQuestSeriesAssetEditor::HandleGraphChanged(const FEdGraphEditAction& GraphEditAction)
{
	if (!bUpdatingGraph && EditingGraph.IsValid())
	{
		EditingGraph->SynchronizeLinksToAsset();
	}
}

void FLxQuestSeriesAssetEditor::DeleteSelectedGraphNodes()
{
	if (!GraphEditor.IsValid() || !EditingGraph.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteGraphNodes", "从关系图移除任务节点"));
	EditingGraph->Modify();
	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet();
	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(SelectedObject); GraphNode && GraphNode->CanUserDeleteNode())
		{
			GraphNode->Modify();
			GraphNode->DestroyNode();
		}
	}
	EditingGraph->SynchronizeLinksToAsset();
	EditingGraph->NotifyGraphChanged();
}

bool FLxQuestSeriesAssetEditor::CanDeleteSelectedGraphNodes() const
{
	return GraphEditor.IsValid() && GraphEditor->GetSelectedNodes().Num() > 0;
}

void FLxQuestSeriesAssetEditor::RemoveInvalidGraphNodes()
{
	if (!EditingAsset.IsValid() || !EditingGraph.IsValid())
	{
		return;
	}

	TGuardValue<bool> UpdatingGraphGuard(bUpdatingGraph, true);
	TArray<UEdGraphNode*> NodesToRemove;
	for (UEdGraphNode* GraphNode : EditingGraph->Nodes)
	{
		const ULxQuestSeriesEdGraphNode* QuestGraphNode = Cast<ULxQuestSeriesEdGraphNode>(GraphNode);
		if (!QuestGraphNode || !EditingAsset->ContainsEditorNode(QuestGraphNode->GetQuestEditorNodeId()))
		{
			NodesToRemove.Add(GraphNode);
		}
	}

	if (!NodesToRemove.IsEmpty())
	{
		EditingGraph->Modify();
		for (UEdGraphNode* GraphNode : NodesToRemove)
		{
			GraphNode->Modify();
			GraphNode->DestroyNode();
		}
		EditingGraph->MarkPackageDirty();
	}
}

void FLxQuestSeriesAssetEditor::RefreshEditor()
{
	RemoveInvalidGraphNodes();
	if (QuestNodeTable.IsValid())
	{
		QuestNodeTable->RefreshItems();
	}
	if (EditingGraph.IsValid())
	{
		EditingGraph->SynchronizeLinksToAsset();
		EditingGraph->NotifyGraphChanged();
	}
}

#undef LOCTEXT_NAMESPACE
