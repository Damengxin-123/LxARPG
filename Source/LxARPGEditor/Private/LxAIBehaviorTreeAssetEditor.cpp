#include "LxAIBehaviorTreeAssetEditor.h"

#include "LxAIBehaviorTreeEdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Docking/TabManager.h"
#include "ScopedTransaction.h"
#include "UObject/UnrealType.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void FLxAIBehaviorTreeAssetEditor::Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxAIBehaviorTreeAsset* Asset)
{
	EditingAsset = Asset;
	Asset->SetFlags(RF_Transactional);
	if (!Asset->EditorGraph)
	{
		Asset->Modify();
		Asset->EditorGraph = NewObject<ULxAIBehaviorTreeEdGraph>(Asset, TEXT("AI行为树图"), RF_Transactional);
		Asset->EditorGraph->Schema = ULxAIBehaviorTreeEdGraphSchema::StaticClass();
	}
	EditingGraph = CastChecked<ULxAIBehaviorTreeEdGraph>(Asset->EditorGraph);
	for (const UEdGraphNode* RawNode : EditingGraph->Nodes)
	{
		const ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (Node && Node->bStart) { bMigratedLegacyGraph = true; break; }
	}
	EditingGraph->EnsureEntryNodes();
	Commands = MakeShared<FUICommandList>();
	Commands->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &FLxAIBehaviorTreeAssetEditor::DeleteSelectedNodes));
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("LxAIControlConfig_Layout_v4")
		->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
		->Split(FTabManager::NewStack()->SetSizeCoefficient(0.72f)->SetHideTabWell(false)->AddTab("LxAIBehaviorTree_Main", ETabState::OpenedTab))
		->Split(FTabManager::NewStack()->SetSizeCoefficient(0.28f)->SetHideTabWell(false)
			->AddTab("LxAIControlConfig_Perception", ETabState::OpenedTab)
			->AddTab("LxAIControlConfig_Analysis", ETabState::OpenedTab)
			->AddTab("LxAIControlConfig_Movement", ETabState::OpenedTab)));
	InitAssetEditor(Mode, Host, TEXT("LxAIBehaviorTreeEditor"), Layout, true, true, Asset);
	if (GraphEditor) GraphEditor->ZoomToFit(false);
	GEditor->RegisterForUndo(this);
}

void FLxAIBehaviorTreeAssetEditor::FocusState(ELxAIBehaviorState State)
{
	if (!GraphEditor || !EditingGraph.IsValid()) return;
	GraphEditor->ClearSelectionSet();
	ULxAIBehaviorTreeNodeData* DetailData = nullptr;
	TSet<const UEdGraphNode*> StateNodes;
	for (UEdGraphNode* Node : EditingGraph->Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* TreeNode = Cast<ULxAIBehaviorTreeEdGraphNode>(Node);
		if (!TreeNode || TreeNode->bStart || !TreeNode->Data || TreeNode->Data->Kind == ELxAIBehaviorNodeKind::Entry
			|| TreeNode->Data->GetState() != State) continue;
		GraphEditor->SetNodeSelection(TreeNode, true);
		if (TreeNode->Data->Kind == ELxAIBehaviorNodeKind::State) StateNodes.Add(TreeNode);
		if (!DetailData || TreeNode->Data->Kind == ELxAIBehaviorNodeKind::Phase)
			DetailData = TreeNode->Data;
	}
	for (UEdGraphNode* RawNode : EditingGraph->Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* EntryNode = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (!EntryNode || !EntryNode->Data || EntryNode->Data->Kind != ELxAIBehaviorNodeKind::Entry) continue;
		for (const UEdGraphPin* Pin : EntryNode->Pins)
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
				if (Linked && StateNodes.Contains(Linked->GetOwningNode())) GraphEditor->SetNodeSelection(EntryNode, true);
	}
	if (DetailData && NodeDetails) NodeDetails->SetObject(DetailData);
	GraphEditor->ZoomToFit(true);
}

void FLxAIBehaviorTreeAssetEditor::FocusEntries()
{
	if (!GraphEditor || !EditingGraph.IsValid()) return;
	GraphEditor->ClearSelectionSet();
	for (UEdGraphNode* RawNode : EditingGraph->Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (Node && Node->Data && Node->Data->Kind == ELxAIBehaviorNodeKind::Entry)
			GraphEditor->SetNodeSelection(Node, true);
	}
	GraphEditor->ZoomToFit(true);
}

void FLxAIBehaviorTreeAssetEditor::FocusAnalysisTab()
{
	if (GetTabManager().IsValid()) GetTabManager()->TryInvokeTab(FTabId(FName(TEXT("LxAIControlConfig_Analysis"))));
}

FLxAIBehaviorTreeAssetEditor::~FLxAIBehaviorTreeAssetEditor()
{
	if (GEditor) GEditor->UnregisterForUndo(this);
}

void FLxAIBehaviorTreeAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	FAssetEditorToolkit::RegisterTabSpawners(Manager);
	Manager->RegisterTabSpawner("LxAIBehaviorTree_Main", FOnSpawnTab::CreateSP(this, &FLxAIBehaviorTreeAssetEditor::SpawnMainTab))
		.SetDisplayName(FText::FromString(TEXT("行为配置")));
	Manager->RegisterTabSpawner("LxAIControlConfig_Perception", FOnSpawnTab::CreateSP(this, &FLxAIBehaviorTreeAssetEditor::SpawnPerceptionTab))
		.SetDisplayName(FText::FromString(TEXT("感知能力")));
	Manager->RegisterTabSpawner("LxAIControlConfig_Movement", FOnSpawnTab::CreateSP(this, &FLxAIBehaviorTreeAssetEditor::SpawnMovementTab))
		.SetDisplayName(FText::FromString(TEXT("运动能力")));
	Manager->RegisterTabSpawner("LxAIControlConfig_Analysis", FOnSpawnTab::CreateSP(this, &FLxAIBehaviorTreeAssetEditor::SpawnAnalysisTab))
		.SetDisplayName(FText::FromString(TEXT("事件分析")));
}

void FLxAIBehaviorTreeAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	Manager->UnregisterTabSpawner("LxAIBehaviorTree_Main");
	Manager->UnregisterTabSpawner("LxAIControlConfig_Perception");
	Manager->UnregisterTabSpawner("LxAIControlConfig_Analysis");
	Manager->UnregisterTabSpawner("LxAIControlConfig_Movement");
	FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}

TSharedRef<SDockTab> FLxAIBehaviorTreeAssetEditor::SpawnPerceptionTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.bLockable = false;
	PerceptionDetails = Properties.CreateDetailView(DetailsArgs);
	PerceptionDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		if (Property.Property.GetFName() == TEXT("Perception")) return true;
		for (const FProperty* Parent : Property.ParentProperties)
			if (Parent->GetFName() == TEXT("Perception")) return true;
		return false;
	}));
	PerceptionDetails->SetObject(EditingAsset.Get());
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("角色类型共用配置，例如宝箱怪。所有使用此资产的角色共用这些参数。"))).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock).Text(this, &FLxAIBehaviorTreeAssetEditor::GetPerceptionValidationText).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().FillHeight(1)[PerceptionDetails.ToSharedRef()]
	];
}

TSharedRef<SDockTab> FLxAIBehaviorTreeAssetEditor::SpawnMovementTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("移动速度已统一到角色蓝图的“角色运动组件 → 移动速度配置”。玩家和AI使用同一套三档倍率，行为树节点只选择运动类型。"))).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock).Text(this, &FLxAIBehaviorTreeAssetEditor::GetMovementValidationText).AutoWrapText(true)
		]
	];
}

FText FLxAIBehaviorTreeAssetEditor::GetMovementValidationText() const
{
	if (!EditingAsset.IsValid()) return FText();
	const auto& Legacy = EditingAsset->Movement;
	return FText::FromString(FString::Printf(TEXT("旧资产倍率（仅供迁移参考，不再生效）：低速 %.2f，中速 %.2f，高速 %.2f。若之前自定义过，请填写到对应角色的运动组件中。"),
		Legacy.LowSpeedMultiplier, Legacy.MediumSpeedMultiplier, Legacy.HighSpeedMultiplier));
}

TSharedRef<SDockTab> FLxAIBehaviorTreeAssetEditor::SpawnAnalysisTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.bLockable = false;
	AnalysisDetails = Properties.CreateDetailView(DetailsArgs);
	AnalysisDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		if (Property.Property.GetFName() == TEXT("Analysis")) return true;
		for (const FProperty* Parent : Property.ParentProperties)
			if (Parent->GetFName() == TEXT("Analysis")) return true;
		return false;
	}));
	AnalysisDetails->SetObject(EditingAsset.Get());
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("分析感知事实与受击通知，按入口优先级选路；靠近使用进入/退出距离避免边界反复切换。运行时按选中阶段的行为顺序执行。"))).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock).Text(this, &FLxAIBehaviorTreeAssetEditor::GetAnalysisValidationText).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().FillHeight(1)[AnalysisDetails.ToSharedRef()]
	];
}

FText FLxAIBehaviorTreeAssetEditor::GetAnalysisValidationText() const
{
	FText Error;
	if (EditingAsset.IsValid() && !EditingAsset->Analysis.ValidateConfiguration(Error))
		return FText::Format(NSLOCTEXT("AI控制配置", "分析错误", "分析配置待修正：{0}"), Error);
	return NSLOCTEXT("AI控制配置", "分析有效", "分析参数有效 · 五个入口的优先级可在图节点详情调整。");
}

FText FLxAIBehaviorTreeAssetEditor::GetPerceptionValidationText() const
{
	FText Error;
	if (EditingAsset.IsValid() && !EditingAsset->Perception.ValidateConfiguration(Error))
		return FText::Format(NSLOCTEXT("AI控制配置", "感知错误", "感知配置待修正：{0}"), Error);
	return NSLOCTEXT("AI控制配置", "感知有效", "感知参数有效 · 距离：米，时间：秒；记忆时间0表示不因超时遗忘。");
}

TSharedRef<SDockTab> FLxAIBehaviorTreeAssetEditor::SpawnMainTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	NodeDetails = Properties.CreateDetailView(DetailsArgs);
	NodeDetails->OnFinishedChangingProperties().AddSP(this, &FLxAIBehaviorTreeAssetEditor::OnPropertyChanged);
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FLxAIBehaviorTreeAssetEditor::OnSelectionChanged);
	GraphEditor = SNew(SGraphEditor).GraphToEdit(EditingGraph.Get()).GraphEvents(Events)
		.AdditionalCommands(Commands).IsEditable(true).ShowGraphStateOverlay(false);
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("五个事件入口按优先级选路；角色死亡入口优先处理生命值归零，执行进入死亡后再启动死亡流程。状态与行为可被多个入口或阶段共享。"))).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock).Text(this, &FLxAIBehaviorTreeAssetEditor::GetValidationText).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock).Visibility_Lambda([this]() { return bMigratedLegacyGraph ? EVisibility::Visible : EVisibility::Collapsed; })
				.Text(FText::FromString(TEXT("旧开始节点已迁移为五个事件入口，原状态、阶段、行为及画布位置已保留；请复核入口连线并保存。"))).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.27f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("节点配置 · 点击节点编辑")))]
				+ SVerticalBox::Slot().FillHeight(1)[NodeDetails.ToSharedRef()]
			]
			+ SSplitter::Slot().Value(0.73f)[GraphEditor.ToSharedRef()]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("从输出引脚拖出只显示可接类型 · 右键空白处显示全部 · Alt+点击引脚断线 · Delete删除 · Ctrl+Z撤销 · Ctrl+S保存"))).AutoWrapText(true)
		]
	];
}

void FLxAIBehaviorTreeAssetEditor::OnSelectionChanged(const TSet<UObject*>& Selection)
{
	UObject* DetailObject = nullptr;
	SelectedGraphNode.Reset();
	for (UObject* Object : Selection)
	{
		if (ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(Object))
		{
			if (!Node->bStart) { DetailObject = Node->Data; SelectedGraphNode = Node; }
			break;
		}
	}
	if (NodeDetails) NodeDetails->SetObject(DetailObject);
}

void FLxAIBehaviorTreeAssetEditor::OnPropertyChanged(const FPropertyChangedEvent& Event)
{
	if (EditingGraph.IsValid())
	{
		ULxAIBehaviorTreeEdGraphNode* ChangedNode = SelectedGraphNode.Get();
		if (ChangedNode && ChangedNode->Data->Kind == ELxAIBehaviorNodeKind::State
			&& (Event.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ULxAIBehaviorTreeNodeData, HealthRange)
				|| (Event.MemberProperty && Event.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ULxAIBehaviorTreeNodeData, HealthRange))))
		{
			for (const FGuid& ChildId : ChangedNode->Data->Children)
			{
				ULxAIBehaviorTreeNodeData* Phase = EditingAsset->FindNode(ChildId);
				if (!Phase || Phase->Kind != ELxAIBehaviorNodeKind::Phase) continue;
				Phase->Modify();
				Phase->HealthRange.Min = FMath::Clamp(Phase->HealthRange.Min, ChangedNode->Data->HealthRange.Min, ChangedNode->Data->HealthRange.Max);
				Phase->HealthRange.Max = FMath::Clamp(Phase->HealthRange.Max, ChangedNode->Data->HealthRange.Min, ChangedNode->Data->HealthRange.Max);
				if (Phase->HealthRange.Min > Phase->HealthRange.Max) Phase->HealthRange.Min = Phase->HealthRange.Max;
			}
			NodeDetails->ForceRefresh();
		}
		EditingGraph->SynchronizeAsset();
		EditingGraph->NotifyGraphChanged();
	}
}

void FLxAIBehaviorTreeAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditor || !EditingGraph.IsValid() || !EditingAsset.IsValid()) return;
	const FScopedTransaction Transaction(FText::FromString(TEXT("删除AI行为树节点")));
	EditingGraph->Modify();
	EditingAsset->Modify();
	const TSet<UObject*> Selection = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet();
	for (UObject* Object : Selection)
	{
		ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(Object);
		if (!Node || !Node->CanUserDeleteNode()) continue;
		Node->Modify();
		EditingGraph->GetSchema()->BreakNodeLinks(*Node);
		Node->DestroyNode();
	}
	EditingGraph->SynchronizeAsset();
	EditingGraph->NotifyGraphChanged();
}

void FLxAIBehaviorTreeAssetEditor::PostUndo(bool bSuccess)
{
	if (!EditingGraph.IsValid()) return;
	if (GraphEditor) GraphEditor->ClearSelectionSet();
	EditingGraph->SynchronizeAsset();
	EditingGraph->NotifyGraphChanged();
	if (NodeDetails) NodeDetails->ForceRefresh();
	if (PerceptionDetails) PerceptionDetails->ForceRefresh();
	if (AnalysisDetails) AnalysisDetails->ForceRefresh();
	if (MovementDetails) MovementDetails->ForceRefresh();
}

FText FLxAIBehaviorTreeAssetEditor::GetValidationText() const
{
	FText Error;
	if (EditingAsset.IsValid() && !EditingAsset->ValidateTree(Error))
		return FText::Format(NSLOCTEXT("AI行为树编辑器", "校验错误", "行为图待完成：{0}"), Error);
	return NSLOCTEXT("AI行为树编辑器", "校验通过", "行为图有效 · 运行时按阶段顺序执行行为节点");
}
