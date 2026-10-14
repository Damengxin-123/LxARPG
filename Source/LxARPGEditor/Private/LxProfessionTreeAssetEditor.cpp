#include "LxProfessionTreeAssetEditor.h"
#include "LxProfessionTreeEdGraph.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "GameplayTagsEditorModule.h"
#include "ScopedTransaction.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"

/** 三列可编辑职业列表行，修改直接写入唯一的静态定义。 */
class SLxProfessionTreeRow : public SMultiColumnTableRow<ULxProfessionTreeDefinition*>
{
public:
	SLATE_BEGIN_ARGS(SLxProfessionTreeRow) {}
		SLATE_ARGUMENT(ULxProfessionTreeDefinition*, Definition)
		SLATE_EVENT(FSimpleDelegate, OnSelect)
	SLATE_END_ARGS()
	/** 初始化对应的职业和列表拥有者。 */
	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
	{
		Definition = Args._Definition;
		OnSelect = Args._OnSelect;
		SMultiColumnTableRow::Construct(FSuperRowType::FArguments().Padding(3.f), Owner);
	}
	/** 先选中行再交由子控件编辑，避免点击文本框后工具栏仍操作上一个职业。 */
	virtual FReply OnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		OnSelect.ExecuteIfBound();
		return FReply::Unhandled();
	}
	/** 按列构建标签选择器或文本编辑框。 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
	{
		if (Column == TEXT("ID"))
			return SNew(SComboButton).ContentPadding(2.f)
				.OnGetMenuContent(this, &SLxProfessionTreeRow::MakeTagPicker)
				.ButtonContent()[SNew(STextBlock).Text_Lambda([this]()
				{ return FText::FromString(Definition.IsValid() && Definition->ProfessionID.IsValid() ? Definition->ProfessionID.ToString() : TEXT("选择职业ID")); })];
		return SNew(SEditableTextBox)
			.Text_Lambda([this, Column]() { return Definition.IsValid() ? (Column == TEXT("Name") ? Definition->DisplayName : Definition->Description) : FText::GetEmpty(); })
			.ToolTipText_Lambda([this, Column]() { return Definition.IsValid() ? (Column == TEXT("Name") ? Definition->DisplayName : Definition->Description) : FText::GetEmpty(); })
			.OnTextCommitted_Lambda([this, Column](const FText& Text, ETextCommit::Type)
			{
				if (!Definition.IsValid()) return;
				FText& Value = Column == TEXT("Name") ? Definition->DisplayName : Definition->Description;
				if (Value.EqualTo(Text)) return;
				const FScopedTransaction Transaction(FText::FromString(TEXT("修改职业信息")));
				Definition->Modify(); Value = Text; Definition->PostEditChange(); Definition->MarkPackageDirty();
			});
	}
private:
	/** 构建职业标签单选菜单。 */
	TSharedRef<SWidget> MakeTagPicker()
	{
		EditedTag = MakeShared<FGameplayTag>(Definition.IsValid() ? Definition->ProfessionID : FGameplayTag());
		return SNew(SBox).WidthOverride(380).HeightOverride(350)
			[IGameplayTagsEditorModule::Get().MakeGameplayTagWidget(FOnSetGameplayTag::CreateSP(this, &SLxProfessionTreeRow::SetTag), EditedTag, TEXT("职业"))];
	}
	/** 写入选中的职业标签；重复配置由实时校验明确提示。 */
	void SetTag(const FGameplayTag& NewTag)
	{
		if (!Definition.IsValid() || Definition->ProfessionID == NewTag) return;
		const FScopedTransaction Transaction(FText::FromString(TEXT("修改职业ID")));
		Definition->Modify(); Definition->ProfessionID = NewTag; Definition->PostEditChange(); Definition->MarkPackageDirty();
	}
	/** 当前行对应的资产内职业对象。 */
	TWeakObjectPtr<ULxProfessionTreeDefinition> Definition;
	/** 标签选择器需要的可编辑值。 */
	TSharedPtr<FGameplayTag> EditedTag;
	/** 行内控件获得鼠标操作前通知编辑器选择职业。 */
	FSimpleDelegate OnSelect;
};

void FLxProfessionTreeAssetEditor::Init(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, ULxProfessionTreeAsset* Asset)
{
	EditingAsset = Asset; Asset->SetFlags(RF_Transactional);
	if (!Asset->EditorGraph)
	{
		Asset->Modify();
		Asset->EditorGraph = NewObject<ULxProfessionTreeEdGraph>(Asset, TEXT("职业关系图"), RF_Transactional);
		Asset->EditorGraph->Schema = ULxProfessionTreeEdGraphSchema::StaticClass();
	}
	EditingGraph = CastChecked<ULxProfessionTreeEdGraph>(Asset->EditorGraph);
	EditingGraph->SetFlags(RF_Transactional);
	for (ULxProfessionTreeDefinition* Definition : Asset->Professions) if (Definition) Definition->SetFlags(RF_Transactional);
	for (ULxProfessionTreeInfluence* Influence : Asset->Influences) if (Influence) Influence->SetFlags(RF_Transactional);
	Commands = MakeShared<FUICommandList>();
	Commands->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &FLxProfessionTreeAssetEditor::DeleteGraphNodes));
	const auto Layout = FTabManager::NewLayout("LxProfessionTree_Layout_v1")
		->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split(FTabManager::NewStack()->SetHideTabWell(true)->AddTab("LxProfessionTree_Main", ETabState::OpenedTab)));
	InitAssetEditor(Mode, Host, TEXT("LxProfessionTreeEditor"), Layout, true, true, Asset);
	GraphHandle = EditingGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FLxProfessionTreeAssetEditor::OnGraphChanged));
	PropertyHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &FLxProfessionTreeAssetEditor::OnPropertyChanged);
	GEditor->RegisterForUndo(this);
	RefreshList(); bRefreshPending = true;
	if (!VisibleProfessions.IsEmpty()) SelectProfession(VisibleProfessions[0]);
}

FLxProfessionTreeAssetEditor::~FLxProfessionTreeAssetEditor()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyHandle);
	if (EditingGraph.IsValid()) EditingGraph->RemoveOnGraphChangedHandler(GraphHandle);
	if (GEditor) GEditor->UnregisterForUndo(this);
}

void FLxProfessionTreeAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	WorkspaceMenuCategory = Manager->AddLocalWorkspaceMenuCategory(FText::FromString(TEXT("职业关系编辑器")));
	FAssetEditorToolkit::RegisterTabSpawners(Manager);
	Manager->RegisterTabSpawner("LxProfessionTree_Main", FOnSpawnTab::CreateSP(this, &FLxProfessionTreeAssetEditor::SpawnMainTab))
		.SetDisplayName(FText::FromString(TEXT("职业关系"))).SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FLxProfessionTreeAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
	Manager->UnregisterTabSpawner("LxProfessionTree_Main"); FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}

TSharedRef<SDockTab> FLxProfessionTreeAssetEditor::SpawnMainTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	FDetailsViewArgs DetailsArgs; DetailsArgs.bHideSelectionTip = true; DetailsArgs.bAllowSearch = true;
	ProfessionDetails = Properties.CreateDetailView(DetailsArgs);
	ProfessionDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		// 旧结构仍供原职业系统使用，仅在新的职业资产编辑器中隐藏逐级手填经验。
		if (Property.Property.GetOwnerStruct() == FLxProfessionLevelEffectConfig::StaticStruct()
			&& Property.Property.GetFName() == GET_MEMBER_NAME_CHECKED(FLxProfessionLevelEffectConfig, RequiredExperience)) return false;
		return Property.Property.GetOwnerClass() != ULxProfessionTreeDefinition::StaticClass()
			|| Property.Property.GetFName() == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeDefinition, Levels)
			|| Property.Property.GetFName() == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeDefinition, FirstLevelExperience)
			|| Property.Property.GetFName() == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeDefinition, Icon)
			|| Property.Property.GetFName() == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeDefinition, ProfessionCategory);
	}));
	FDetailsViewArgs AssetDetailsArgs; AssetDetailsArgs.bHideSelectionTip = true; AssetDetailsArgs.bAllowSearch = false;
	AssetDetails = Properties.CreateDetailView(AssetDetailsArgs);
	AssetDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
	{
		return Property.Property.GetFName() == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeAsset, ExperienceGrowthCoefficient);
	}));
	AssetDetails->SetObject(EditingAsset.Get());
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FLxProfessionTreeAssetEditor::OnGraphSelection);
	GraphEditor = SNew(SGraphEditor).AdditionalCommands(Commands).GraphToEdit(EditingGraph.Get()).GraphEvents(Events).ShowGraphStateOverlay(false);
	TSharedRef<SHorizontalBox> CategoryButtons = SNew(SHorizontalBox);
	for (ELxProfessionTreeCategory Category : { ELxProfessionTreeCategory::Combat, ELxProfessionTreeCategory::Gathering, ELxProfessionTreeCategory::Production })
	{
		CategoryButtons->AddSlot().FillWidth(1).Padding(2)
		[SNew(SButton).HAlign(HAlign_Center)
			.ButtonColorAndOpacity_Lambda([this, Category]() { return ActiveCategory == Category ? FLinearColor(0.12f, 0.4f, 0.5f) : FLinearColor(0.15f, 0.15f, 0.15f); })
			.Text_Lambda([Category]()
			{
				return StaticEnum<ELxProfessionTreeCategory>()->GetDisplayNameTextByValue(static_cast<int64>(Category));
			}).OnClicked(this, &FLxProfessionTreeAssetEditor::ChangeCategory, Category)];
	}
	ProfessionList = SNew(SListView<ULxProfessionTreeDefinition*>).ListItemsSource(&VisibleProfessions).SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &FLxProfessionTreeAssetEditor::GenerateRow)
		.OnSelectionChanged(this, &FLxProfessionTreeAssetEditor::OnListSelection)
		.OnMouseButtonDoubleClick(this, &FLxProfessionTreeAssetEditor::PlaceListItem)
		.HeaderRow(SNew(SHeaderRow)
			+ SHeaderRow::Column("ID").DefaultLabel(FText::FromString(TEXT("职业ID"))).FillWidth(0.35f)
			+ SHeaderRow::Column("Name").DefaultLabel(FText::FromString(TEXT("职业可视化名称"))).FillWidth(0.27f)
			+ SHeaderRow::Column("Description").DefaultLabel(FText::FromString(TEXT("职业描述"))).FillWidth(0.38f));
	return SNew(SDockTab)
	[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
		[SNew(STextBlock).Text(FText::FromString(TEXT("职业关系示例 · 左侧管理职业，中间连接影响，右侧编辑等级与关系参数")))]
		+ SVerticalBox::Slot().FillHeight(1)
		[SNew(SSplitter)
			+ SSplitter::Slot().Value(0.30f)
			[SNew(SBorder).Padding(6)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[SNew(SBox).HeightOverride(110)[AssetDetails.ToSharedRef()]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[SNew(STextBlock).Text(FText::FromString(TEXT("职业列表")))]
					+ SVerticalBox::Slot().AutoHeight()[CategoryButtons]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1).Padding(2)[SNew(SButton).HAlign(HAlign_Center).Text(FText::FromString(TEXT("添加职业"))).OnClicked(this, &FLxProfessionTreeAssetEditor::AddProfession)]
						+ SHorizontalBox::Slot().FillWidth(1).Padding(2)[SNew(SButton).HAlign(HAlign_Center).Text(FText::FromString(TEXT("放置到画布"))).OnClicked(this, &FLxProfessionTreeAssetEditor::PlaceProfession)]
						+ SHorizontalBox::Slot().FillWidth(1).Padding(2)[SNew(SButton).HAlign(HAlign_Center).Text(FText::FromString(TEXT("删除职业"))).OnClicked(this, &FLxProfessionTreeAssetEditor::DeleteProfession)]]
					+ SVerticalBox::Slot().FillHeight(1)[ProfessionList.ToSharedRef()]
					+ SVerticalBox::Slot().AutoHeight().Padding(2, 6)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("双击列表行放置或定位节点。分类可在职业细节中调整。")))]]]
			+ SSplitter::Slot().Value(0.42f)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(6)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("职业关系画布   ● 前置需求   ● 增益／抑制\n右侧“影响” → 左侧需求引脚；Alt+单击引脚断线")))]
				+ SVerticalBox::Slot().FillHeight(1)[GraphEditor.ToSharedRef()]]
			+ SSplitter::Slot().Value(0.28f)
			[SNew(SSplitter).Orientation(Orient_Vertical)
				+ SSplitter::Slot().Value(0.55f)
				[SNew(SBorder).Padding(5)[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(3)[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(TEXT("职业细节配置 · ") + (SelectedProfession.IsValid() ? SelectedProfession->DisplayName.ToString() : TEXT("请选择职业节点"))); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(3)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("升级经验 = y × [1 + z × (x − 1)²]\nx为当前等级；y由当前职业配置，z由左侧职业体系统一配置。")))]
					+ SVerticalBox::Slot().AutoHeight().Padding(3)[SNew(STextBlock).AutoWrapText(true).Text(this, &FLxProfessionTreeAssetEditor::GetExperiencePreviewText)]
					+ SVerticalBox::Slot().FillHeight(1)[ProfessionDetails.ToSharedRef()]]]
				+ SSplitter::Slot().Value(0.45f)
				[SNew(SBorder).Padding(5)[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(3)[SNew(STextBlock).Text(FText::FromString(TEXT("职业影响细节")))]
					+ SVerticalBox::Slot().AutoHeight().Padding(3)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("倍率1＝100%；1.2＝增强20%；0.01＝仅保留1%。相互影响时双方使用同一倍率。")))]
					+ SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(InfluenceBox, SVerticalBox)]]]]]]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 5)
		[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return ValidationText; }).ToolTipText_Lambda([this]() { return ValidationText; })]];
}

TSharedRef<ITableRow> FLxProfessionTreeAssetEditor::GenerateRow(ULxProfessionTreeDefinition* Item, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(SLxProfessionTreeRow, Owner).Definition(Item)
		.OnSelect(FSimpleDelegate::CreateLambda([this, Item]() { if (SelectedProfession.Get() != Item) SelectProfession(Item); }));
}

void FLxProfessionTreeAssetEditor::RefreshList()
{
	TGuardValue<bool> Guard(bSelecting, true);
	VisibleProfessions.Reset();
	if (EditingAsset.IsValid()) for (ULxProfessionTreeDefinition* Definition : EditingAsset->Professions)
		if (Definition && Definition->ProfessionCategory == ActiveCategory) VisibleProfessions.Add(Definition);
	if (ProfessionList)
	{
		ProfessionList->RequestListRefresh();
		if (VisibleProfessions.Contains(SelectedProfession.Get())) ProfessionList->SetSelection(SelectedProfession.Get());
		else ProfessionList->ClearSelection();
	}
}

FReply FLxProfessionTreeAssetEditor::ChangeCategory(ELxProfessionTreeCategory Category)
{
	ActiveCategory = Category; RefreshList(); return FReply::Handled();
}

FReply FLxProfessionTreeAssetEditor::AddProfession()
{
	if (!EditingAsset.IsValid()) return FReply::Handled();
	const FScopedTransaction Transaction(FText::FromString(TEXT("添加职业")));
	EditingAsset->Modify();
	auto* Definition = NewObject<ULxProfessionTreeDefinition>(EditingAsset.Get(), NAME_None, RF_Transactional);
	Definition->ProfessionCategory = ActiveCategory; Definition->DisplayName = FText::FromString(TEXT("新职业")); Definition->Levels.AddDefaulted();
	Definition->RefreshLevelExperience();
	EditingAsset->Professions.Add(Definition); EditingAsset->MarkPackageDirty();
	RefreshList(); SelectProfession(Definition); bRefreshPending = true;
	return FReply::Handled();
}

FReply FLxProfessionTreeAssetEditor::DeleteProfession()
{
	const auto Selection = ProfessionList->GetSelectedItems();
	if (Selection.IsEmpty() || !EditingAsset.IsValid()) return FReply::Handled();
	ULxProfessionTreeDefinition* Definition = Selection[0];
	const FScopedTransaction Transaction(FText::FromString(TEXT("删除职业及其关系")));
	EditingAsset->Modify(); EditingGraph->Modify();
	SelectProfession(nullptr);
	const TArray<TObjectPtr<UEdGraphNode>> Nodes = EditingGraph->Nodes;
	for (UEdGraphNode* Node : Nodes)
		if (auto* Typed = Cast<ULxProfessionTreeEdGraphNode>(Node); Typed && Typed->Definition == Definition)
		{ Typed->Modify(); Typed->DestroyNode(); }
	EditingAsset->Professions.Remove(Definition); EditingGraph->SynchronizeAsset(); EditingGraph->NotifyGraphChanged();
	EditingAsset->MarkPackageDirty(); bRefreshPending = true;
	return FReply::Handled();
}

FReply FLxProfessionTreeAssetEditor::PlaceProfession()
{
	const auto Selection = ProfessionList->GetSelectedItems();
	if (!Selection.IsEmpty()) PlaceListItem(Selection[0]);
	return FReply::Handled();
}

void FLxProfessionTreeAssetEditor::PlaceListItem(ULxProfessionTreeDefinition* Profession)
{
	if (!Profession || !EditingGraph.IsValid()) return;
	FLxProfessionTreePlaceAction Action; Action.Definition = Profession;
	UEdGraphNode* Node = Action.PerformAction(EditingGraph.Get(), nullptr, FVector2f(GraphEditor->GetPasteLocation2f()), true);
	SelectProfession(Profession);
	if (Node) GraphEditor->JumpToNode(Node, false);
}

void FLxProfessionTreeAssetEditor::OnListSelection(ULxProfessionTreeDefinition* Profession, ESelectInfo::Type SelectInfo)
{
	if (!bSelecting && SelectInfo != ESelectInfo::Direct) SelectProfession(Profession);
}

void FLxProfessionTreeAssetEditor::OnGraphSelection(const TSet<UObject*>& Selection)
{
	if (bSelecting) return;
	ULxProfessionTreeDefinition* Definition = nullptr;
	if (Selection.Num() == 1) for (UObject* Object : Selection)
		if (auto* Node = Cast<ULxProfessionTreeEdGraphNode>(Object)) Definition = Node->Definition;
	TGuardValue<bool> Guard(bSelecting, true);
	SelectedProfession = Definition;
	if (Definition) ActiveCategory = Definition->ProfessionCategory;
	RefreshList(); ProfessionDetails->SetObject(Definition); ProfessionDetails->SetRootExpansionStates(true, true); RefreshInfluences();
}

void FLxProfessionTreeAssetEditor::SelectProfession(ULxProfessionTreeDefinition* Profession)
{
	TGuardValue<bool> Guard(bSelecting, true);
	SelectedProfession = Profession;
	if (Profession && ActiveCategory != Profession->ProfessionCategory)
	{ ActiveCategory = Profession->ProfessionCategory; RefreshList(); }
	if (Profession) ProfessionList->SetSelection(Profession, ESelectInfo::Direct);
	else ProfessionList->ClearSelection();
	ProfessionDetails->SetObject(Profession); ProfessionDetails->SetRootExpansionStates(true, true);
	GraphEditor->ClearSelectionSet();
	for (UEdGraphNode* Node : EditingGraph->Nodes)
		if (auto* Typed = Cast<ULxProfessionTreeEdGraphNode>(Node); Typed && Typed->Definition == Profession && Profession)
			GraphEditor->SetNodeSelection(Typed, true);
	RefreshInfluences();
}

void FLxProfessionTreeAssetEditor::DeleteGraphNodes()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("移除画布职业节点")));
	EditingGraph->Modify();
	const TSet<UObject*> Selection = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet();
	for (UObject* Object : Selection) if (auto* Node = Cast<ULxProfessionTreeEdGraphNode>(Object))
	{ Node->Modify(); Node->DestroyNode(); }
	EditingGraph->SynchronizeAsset(); EditingGraph->NotifyGraphChanged(); EditingGraph->MarkPackageDirty();
}

void FLxProfessionTreeAssetEditor::RefreshInfluences()
{
	if (!InfluenceBox) return;
	InfluenceBox->ClearChildren();
	if (!EditingAsset.IsValid() || !SelectedProfession.IsValid()) return;
	int32 RelationCount = 0;
	for (ULxProfessionTreeInfluence* Relation : EditingAsset->GetIncomingInfluences(SelectedProfession.Get()))
	{
		++RelationCount;
		const TWeakObjectPtr<ULxProfessionTreeInfluence> WeakRelation = Relation;
		TSharedRef<SVerticalBox> Parameters = SNew(SVerticalBox);
		if (Relation->Kind == ELxProfessionTreeRelation::Prerequisite)
		{
			Parameters->AddSlot().AutoHeight().Padding(3, 8)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("前置等级要求")))]
				+ SHorizontalBox::Slot().FillWidth(1)
				[SNew(SNumericEntryBox<int32>).MinValue(1).AllowSpin(false)
					.MaxValue_Lambda([WeakRelation]() -> TOptional<int32> { return WeakRelation.IsValid() ? FMath::Max(1, WeakRelation->GetMaxRequiredLevel()) : 1; })
					.IsEnabled_Lambda([WeakRelation]() { return WeakRelation.IsValid() && WeakRelation->GetMaxRequiredLevel() >= 1; })
					.Value_Lambda([WeakRelation]() -> TOptional<int32> { return WeakRelation.IsValid() ? WeakRelation->RequiredLevel : 1; })
					.OnValueCommitted_Lambda([WeakRelation](int32 Value, ETextCommit::Type)
					{
						if (!WeakRelation.IsValid()) return;
						const FScopedTransaction Transaction(FText::FromString(TEXT("修改职业前置等级")));
						WeakRelation->Modify(); WeakRelation->SetRequiredLevel(Value); WeakRelation->PostEditChange(); WeakRelation->MarkPackageDirty();
					})]];
		}
		else
		{
			Parameters->AddSlot().AutoHeight().Padding(3, 8)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("增益倍率")))]
				+ SHorizontalBox::Slot().FillWidth(1)
				[SNew(SNumericEntryBox<float>).MinValue(0.f).AllowSpin(false).MaxFractionalDigits(6)
					.Value_Lambda([WeakRelation]() -> TOptional<float> { return WeakRelation.IsValid() ? WeakRelation->Multiplier : 1.f; })
					.OnValueCommitted_Lambda([WeakRelation](float Value, ETextCommit::Type)
					{
						if (!WeakRelation.IsValid() || !FMath::IsFinite(Value)) return;
						const FScopedTransaction Transaction(FText::FromString(TEXT("修改职业增益倍率")));
						WeakRelation->Modify(); WeakRelation->Multiplier = FMath::Max(0.f, Value); WeakRelation->PostEditChange(); WeakRelation->MarkPackageDirty();
					})]];
			Parameters->AddSlot().AutoHeight().Padding(3, 5)
			[SNew(SCheckBox)
				.ToolTipText(FText::FromString(TEXT("双方使用相同倍率。如果已有反向增益连线，请先断开反向连线。")))
				.IsEnabled_Lambda([WeakRelation]() { return WeakRelation.IsValid() && WeakRelation->CanEditChange(FindFProperty<FProperty>(ULxProfessionTreeInfluence::StaticClass(), GET_MEMBER_NAME_CHECKED(ULxProfessionTreeInfluence, bMutual))); })
				.IsChecked_Lambda([WeakRelation]() { return WeakRelation.IsValid() && WeakRelation->bMutual ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([WeakRelation](ECheckBoxState State)
				{
					if (!WeakRelation.IsValid()) return;
					const FScopedTransaction Transaction(FText::FromString(TEXT("修改职业相互影响")));
					WeakRelation->Modify(); WeakRelation->bMutual = State == ECheckBoxState::Checked; WeakRelation->PostEditChange(); WeakRelation->MarkPackageDirty();
				})[SNew(STextBlock).Text(FText::FromString(TEXT("相互影响（双方使用同一倍率）")))]];
		}
		InfluenceBox->AddSlot().AutoHeight().Padding(2, 5)
		[SNew(SBorder).Padding(4)[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this, WeakRelation]()
			{
				const auto* Current = WeakRelation.Get();
				if (!Current || !Current->Source || !Current->Target) return FText::GetEmpty();
				const auto* Source = Current->Target == SelectedProfession.Get() ? Current->Source.Get() : Current->Target.Get();
				return FText::FromString(TEXT("受到影响：") + Source->DisplayName.ToString() + TEXT(" → ") + SelectedProfession->DisplayName.ToString());
			})]
			+ SVerticalBox::Slot().AutoHeight()[Parameters]]];
	}
	if (RelationCount == 0) InfluenceBox->AddSlot().AutoHeight().Padding(6)
		[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("尚未受到其他职业影响。将其他职业的输出连接到该节点左侧的前置或增益引脚。")))];
}

void FLxProfessionTreeAssetEditor::OnPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (!EditingAsset.IsValid() || !Object || (Object != EditingAsset.Get() && !Object->IsIn(EditingAsset.Get()))) return;
	bRefreshPending = true;
	if (Object->IsA<ULxProfessionTreeInfluence>()) bInfluencesPending = true;
	// 定义属性可能改变节点名称或分类颜色；影响参数和节点坐标变化不需要重建节点控件。
	if (Object->IsA<ULxProfessionTreeDefinition>())
	{
		const FName Name = Event.GetMemberPropertyName();
		if (Name.IsNone() || Name == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeDefinition, DisplayName)
			|| Name == GET_MEMBER_NAME_CHECKED(ULxProfessionTreeDefinition, ProfessionCategory)) bGraphRefreshPending = true;
	}
}

void FLxProfessionTreeAssetEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	bRefreshPending = true; bInfluencesPending = true;
}

void FLxProfessionTreeAssetEditor::PostUndo(bool bSuccess)
{
	if (!EditingAsset.IsValid()) return;
	if (!EditingAsset->Professions.Contains(SelectedProfession.Get())) SelectProfession(nullptr);
	if (ProfessionDetails) ProfessionDetails->ForceRefresh();
	if (AssetDetails) AssetDetails->ForceRefresh();
	bRefreshPending = true; bInfluencesPending = true; bGraphRefreshPending = true;
}

void FLxProfessionTreeAssetEditor::Tick(float DeltaTime)
{
	if (GraphEditor && InitialLayoutFrames < 4 && GraphEditor->GetCachedGeometry().GetLocalSize().X > 100)
	{
		if (++InitialLayoutFrames == 4) FrameGraph();
	}
	if (!bRefreshPending || !EditingAsset.IsValid() || !ProfessionList) return;
	// NotifyGraphChanged会同步回调OnGraphChanged，必须先消费独立的画布请求，
	// 再消费面板刷新请求，防止每一帧清空并重建正在接受鼠标事件的节点。
	if (bGraphRefreshPending)
	{
		bGraphRefreshPending = false;
		GraphEditor->NotifyGraphChanged();
	}
	bRefreshPending = false;
	RefreshList();
	if (bInfluencesPending) { bInfluencesPending = false; RefreshInfluences(); }
	const TArray<FText> Errors = EditingAsset->ValidateConfiguration();
	ValidationText = Errors.IsEmpty() ? FText::FromString(TEXT("配置检查通过 · 在数据管理配置中指定此资产作为职业体系"))
		: FText::Format(FText::FromString(TEXT("配置提示（{0}项）：{1}")), FText::AsNumber(Errors.Num()), Errors[0]);
}

void FLxProfessionTreeAssetEditor::FrameGraph()
{
	if (GraphEditor) GraphEditor->ZoomToFit(false);
}

FText FLxProfessionTreeAssetEditor::GetExperiencePreviewText() const
{
	const ULxProfessionTreeDefinition* Definition = SelectedProfession.Get();
	if (!Definition) return FText::GetEmpty();
	if (Definition->Levels.Num() <= 1) return FText::FromString(TEXT("经验预览：当前没有可升级的等级。"));
	TArray<FString> Items;
	FNumberFormattingOptions Format;
	Format.SetMaximumFractionalDigits(2);
	const int32 LastUpgradeableLevel = Definition->Levels.Num() - 1;
	for (int32 Level = 1; Level <= FMath::Min(LastUpgradeableLevel, 5); ++Level)
		Items.Add(FString::Printf(TEXT("%d→%d：%s"), Level, Level + 1,
			*FText::AsNumber(Definition->GetRequiredExperienceForNextLevel(Level), &Format).ToString()));
	if (LastUpgradeableLevel > 5)
		Items.Add(FString::Printf(TEXT("… %d→%d：%s"), LastUpgradeableLevel, LastUpgradeableLevel + 1,
			*FText::AsNumber(Definition->GetRequiredExperienceForNextLevel(LastUpgradeableLevel), &Format).ToString()));
	return FText::FromString(TEXT("经验预览：") + FString::Join(Items, TEXT("；")) + TEXT("。满级后不再升级。"));
}
