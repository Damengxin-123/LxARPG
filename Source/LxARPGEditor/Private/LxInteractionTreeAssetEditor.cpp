#include "LxInteractionTreeAssetEditor.h"
#include "LxInteractionTreeEdGraph.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "IPropertyTypeCustomization.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "GameplayTagsManager.h"
#include "SGameplayTagPicker.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
/** 仅在交互树任务面板中按系列限制任务标签的选择。 */
class FLxQuestInteractionConfigCustomization : public IPropertyTypeCustomization
{
public:
	/** 保留任务配置的默认标题。 */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row,
		IPropertyTypeCustomizationUtils& Utils) override
	{
		Row.NameContent()[Handle->CreatePropertyNameWidget()];
	}

	/** 保留普通配置行，为任务 ID 创建受系列约束的选择器。 */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Children,
		IPropertyTypeCustomizationUtils& Utils) override
	{
		const TSharedRef<IPropertyHandle> Series = Handle->GetChildHandle(TEXT("QuestSeriesId")).ToSharedRef();
		const TSharedRef<IPropertyHandle> Quest = Handle->GetChildHandle(TEXT("QuestId")).ToSharedRef();
		Children.AddProperty(Handle->GetChildHandle(TEXT("bUseQuestDisplayText")).ToSharedRef());
		Children.AddProperty(Series);
		Series->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([Series, Quest]()
		{
			const FGameplayTag SeriesTag = ReadTag(Series);
			const FGameplayTag QuestTag = ReadTag(Quest);
			if (QuestTag.IsValid() && (!SeriesTag.IsValid() || QuestTag == SeriesTag || !QuestTag.MatchesTag(SeriesTag)))
			{
				Quest->SetValueFromFormattedString(TEXT("(TagName=\"\")"));
			}
		}));
		Children.AddCustomRow(Quest->GetPropertyDisplayName())
		.NameContent()[Quest->CreatePropertyNameWidget()]
		.ValueContent().MinDesiredWidth(250)
		[
			SNew(SComboButton)
			.IsEnabled_Lambda([Series, Quest]() { return ReadTag(Series).IsValid() && !Quest->IsEditConst(); })
			.OnGetMenuContent_Lambda([Series, Quest]() -> TSharedRef<SWidget>
			{
				// 以直接子标签作为树根，显示所有后代但不暴露系列自身。
				const TSharedPtr<FGameplayTagNode> SeriesNode = UGameplayTagsManager::Get().FindTagNode(ReadTag(Series));
				TArray<FString> Roots;
				if (SeriesNode.IsValid())
				{
					for (const TSharedPtr<FGameplayTagNode>& Child : SeriesNode->GetChildTagNodes())
					{
						Roots.Add(Child->GetCompleteTag().ToString());
					}
				}
				// 空筛选会显示整个标签树，因此无子项时只显示提示。
				if (Roots.IsEmpty())
				{
					return SNew(STextBlock).Text(FText::FromString(TEXT("当前任务系列没有子标签")));
				}
				return SNew(SGameplayTagPicker)
					.Filter(FString::Join(Roots, TEXT(",")))
					.MultiSelect(false)
					.PropertyHandle(Quest)
					.MaxHeight(350.0f);
			})
			.ButtonContent()
			[
				SNew(STextBlock).Text_Lambda([Series, Quest]()
				{
					const FGameplayTag Tag = ReadTag(Quest);
					return FText::FromString(Tag.IsValid() ? Tag.ToString()
						: ReadTag(Series).IsValid() ? TEXT("选择任务标签") : TEXT("请先选择任务系列ID"));
				})
			]
		];
	}

private:
	/** 从属性句柄读取当前单个标签，避免保留可失效的数据指针。 */
	static FGameplayTag ReadTag(const TSharedRef<IPropertyHandle>& Handle)
	{
		FName Name;
		const TSharedPtr<IPropertyHandle> NameHandle = Handle->GetChildHandle(TEXT("TagName"));
		return NameHandle.IsValid() && NameHandle->GetValue(Name) == FPropertyAccess::Success
			? FGameplayTag::RequestGameplayTag(Name, false) : FGameplayTag();
	}
};
}

void FLxInteractionTreeAssetEditor::Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxInteractionTreeAsset* Asset)
{
	EditingAsset = Asset;
	Asset->SetFlags(RF_Transactional);
	if (!Asset->EditorGraph)
	{
		Asset->EditorGraph = NewObject<ULxInteractionTreeEdGraph>(Asset, TEXT("交互树图"), RF_Transactional);
		Asset->EditorGraph->Schema = ULxInteractionTreeEdGraphSchema::StaticClass();
	}
	EditingGraph = CastChecked<ULxInteractionTreeEdGraph>(Asset->EditorGraph);
	EditingGraph->EnsureStartNode();
	Commands = MakeShared<FUICommandList>();
	Commands->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &FLxInteractionTreeAssetEditor::DeleteSelectedNodes));
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("LxInteractionTree_Layout_v1")
		->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split(FTabManager::NewStack()->SetHideTabWell(true)->AddTab("LxInteractionTree_Main", ETabState::OpenedTab)));
	InitAssetEditor(Mode, Host, TEXT("LxInteractionTreeEditor"), Layout, true, true, Asset);
	GraphEditor->ZoomToFit(false);
	for (UEdGraphNode* Node : EditingGraph->Nodes)
	{
		ULxInteractionTreeEdGraphNode* TreeNode = Cast<ULxInteractionTreeEdGraphNode>(Node);
		if (TreeNode && TreeNode->Data && Asset->Roots.Contains(TreeNode->Data->NodeId))
		{
			GraphEditor->SetNodeSelection(TreeNode, true);
			break;
		}
	}
	GEditor->RegisterForUndo(this);
}

FLxInteractionTreeAssetEditor::~FLxInteractionTreeAssetEditor()
{
	if (GEditor) GEditor->UnregisterForUndo(this);
}

void FLxInteractionTreeAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	FAssetEditorToolkit::RegisterTabSpawners(Manager);
	Manager->RegisterTabSpawner("LxInteractionTree_Main", FOnSpawnTab::CreateSP(this, &FLxInteractionTreeAssetEditor::SpawnMainTab))
		.SetDisplayName(FText::FromString(TEXT("交互树")));
}

void FLxInteractionTreeAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	Manager->UnregisterTabSpawner("LxInteractionTree_Main");
	FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}

TSharedRef<SDockTab> FLxInteractionTreeAssetEditor::SpawnMainTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	NodeDetails = Properties.CreateDetailView(DetailsArgs);
	NodeFeatureDetails = Properties.CreateDetailView(DetailsArgs);
	NodeFeatureDetails->RegisterInstancedCustomPropertyTypeLayout(TEXT("LxQuestInteractionConfig"),
		FOnGetPropertyTypeCustomizationInstance::CreateLambda([]()
		{
			return MakeShared<FLxQuestInteractionConfigCustomization>();
		}));
	FeatureDetails = Properties.CreateDetailView(DetailsArgs);
	// 相同节点的通用字段和任务字段分别在两个面板显示。
	NodeDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		if (Property.Property.GetFName() == TEXT("QuestConfig")) return false;
		for (const FProperty* Parent : Property.ParentProperties)
			if (Parent->GetFName() == TEXT("QuestConfig")) return false;
		return true;
	}));
	NodeFeatureDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		if (Property.Property.GetFName() == TEXT("QuestConfig")) return true;
		for (const FProperty* Parent : Property.ParentProperties)
			if (Parent->GetFName() == TEXT("QuestConfig")) return true;
		return false;
	}));
	for (const TSharedPtr<IDetailsView>& View : {NodeDetails, NodeFeatureDetails, FeatureDetails})
		View->OnFinishedChangingProperties().AddSP(this, &FLxInteractionTreeAssetEditor::OnPropertyChanged);
	FeatureDetails->SetObject(EditingAsset.Get());
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FLxInteractionTreeAssetEditor::OnSelectionChanged);
	GraphEditor = SNew(SGraphEditor).GraphToEdit(EditingGraph.Get()).GraphEvents(Events)
		.AdditionalCommands(Commands).IsEditable(true).ShowGraphStateOverlay(false);
	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(this, &FLxInteractionTreeAssetEditor::GetValidationText).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.23f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("节点细节 · 点击节点编辑")))]
				+ SVerticalBox::Slot().FillHeight(1)[NodeDetails.ToSharedRef()]
			]
			+ SSplitter::Slot().Value(0.52f)[GraphEditor.ToSharedRef()]
			+ SSplitter::Slot().Value(0.25f)
			[
				SNew(SSplitter).Orientation(Orient_Vertical)
				+ SSplitter::Slot().Value(0.7f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("功能开关与共享配置")))]
					+ SVerticalBox::Slot().FillHeight(1)[FeatureDetails.ToSharedRef()]
				]
				+ SSplitter::Slot().Value(0.3f)
				[
					SNew(SVerticalBox)
					.Visibility_Lambda([this]() { return bShowNodeFeatureDetails ? EVisibility::Visible : EVisibility::Collapsed; })
					+ SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(FText::FromString(TEXT("当前节点功能参数 · 任务")))]
					+ SVerticalBox::Slot().FillHeight(1)[NodeFeatureDetails.ToSharedRef()]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("从开始节点向右连接 · 右键空白处添加节点 · Alt+点击引脚断开连线 · Delete删除 · Ctrl+Z撤销 · Ctrl+S保存")))
		]
	];
}

void FLxInteractionTreeAssetEditor::OnSelectionChanged(const TSet<UObject*>& Selection)
{
	UObject* Data = nullptr;
	for (UObject* Object : Selection)
	{
		if (ULxInteractionTreeEdGraphNode* Node = Cast<ULxInteractionTreeEdGraphNode>(Object))
		{
			if (!Node->bStart) Data = Node->Data;
			break;
		}
	}
	NodeDetails->SetObject(Data);
	const ULxInteractionTreeNodeData* NodeData = Cast<ULxInteractionTreeNodeData>(Data);
	bShowNodeFeatureDetails = NodeData && NodeData->Type == ELxInteractionActionType::Quest;
	NodeFeatureDetails->SetObject(Data);
}

void FLxInteractionTreeAssetEditor::OnPropertyChanged(const FPropertyChangedEvent& Event)
{
	if (EditingGraph.IsValid())
	{
		EditingGraph->SynchronizeAsset();
		EditingGraph->NotifyGraphChanged();
	}
}

void FLxInteractionTreeAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditor || !EditingGraph.IsValid()) return;
	const FScopedTransaction Transaction(FText::FromString(TEXT("删除交互节点")));
	EditingGraph->Modify();
	EditingAsset->Modify();
	const TSet<UObject*> Selection = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet();
	for (UObject* Object : Selection)
	{
		ULxInteractionTreeEdGraphNode* Node = Cast<ULxInteractionTreeEdGraphNode>(Object);
		if (!Node || !Node->CanUserDeleteNode()) continue;
		Node->Modify();
		EditingGraph->GetSchema()->BreakNodeLinks(*Node);
		Node->DestroyNode();
	}
	EditingGraph->SynchronizeAsset();
	EditingGraph->NotifyGraphChanged();
}

void FLxInteractionTreeAssetEditor::PostUndo(bool bSuccess)
{
	if (EditingGraph.IsValid())
	{
		GraphEditor->ClearSelectionSet();
		EditingGraph->SynchronizeAsset();
		EditingGraph->NotifyGraphChanged();
		NodeDetails->ForceRefresh();
		FeatureDetails->ForceRefresh();
		NodeFeatureDetails->ForceRefresh();
	}
}

FText FLxInteractionTreeAssetEditor::GetValidationText() const
{
	FText Error;
	if (EditingAsset.IsValid() && !EditingAsset->ValidateTree(Error))
		return FText::Format(NSLOCTEXT("交互树", "校验错误", "配置待完成：{0}"), Error);
	return NSLOCTEXT("交互树", "校验通过", "配置有效 · 保存后可在NPC的可交互组件中指定此资产");
}
