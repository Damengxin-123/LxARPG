#include "Widgets/LxQuestNodeTable.h"

#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "GameplayTagsEditorModule.h"
#include "SGameplayTagPicker.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "LxQuestNodeTable"

namespace LxQuestNodeTableColumns
{
	const FName QuestId(TEXT("QuestId"));
	const FName DeveloperName(TEXT("DeveloperName"));
	const FName DisplayName(TEXT("DisplayName"));
	const FName Description(TEXT("Description"));
	const FName QuestContent(TEXT("QuestContent"));
}

/** 把一个任务节点定义呈现为可直接编辑的多列表格行。 */
class SLxQuestNodeTableRow : public SMultiColumnTableRow<TSharedPtr<FLxQuestNodeListItem>>
{
public:
	SLATE_BEGIN_ARGS(SLxQuestNodeTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FLxQuestNodeListItem>, Item)
		SLATE_ARGUMENT(ULxQuestSeriesAsset*, QuestSeriesAsset)
		SLATE_EVENT(FLxOnQuestNodeTableChanged, OnNodeChanged)
	SLATE_END_ARGS()

	/** 保存任务节点引用并构建标准多列表格行。 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Item = InArgs._Item;
		QuestSeriesAsset = InArgs._QuestSeriesAsset;
		OnNodeChanged = InArgs._OnNodeChanged;
		SMultiColumnTableRow<TSharedPtr<FLxQuestNodeListItem>>::Construct(
			FSuperRowType::FArguments().Padding(FMargin(2.0f)), OwnerTable);
	}

	/** 根据列名称创建任务节点字段控件。 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == LxQuestNodeTableColumns::QuestId)
		{
			return SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.IsEnabled_Lambda([this]()
				{
					return QuestSeriesAsset.IsValid() && QuestSeriesAsset->GetQuestSeriesId().IsValid();
				})
				.ToolTipText(LOCTEXT("QuestIdPickerTooltip", "只能选择当前任务系列ID的子标签；请先选择任务系列ID。"))
				.OnGetMenuContent(this, &SLxQuestNodeTableRow::CreateQuestIdPicker)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const FLxQuestNodeDefinition* QuestNode = FindQuestNode();
						return QuestNode && QuestNode->QuestId.IsValid()
							? FText::FromName(QuestNode->QuestId.GetTagName())
							: LOCTEXT("SelectQuestId", "选择任务ID");
					})
				];
		}

		if (ColumnName == LxQuestNodeTableColumns::DeveloperName)
		{
			return SNew(SEditableTextBox)
				.Text_Lambda([this]()
				{
					const FLxQuestNodeDefinition* QuestNode = FindQuestNode();
					return QuestNode ? FText::FromName(QuestNode->DeveloperName) : FText::GetEmpty();
				})
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
				{
					EditQuestNode(LOCTEXT("EditDeveloperName", "修改任务节点开发名称"),
						[&NewText](FLxQuestNodeDefinition& QuestNode)
						{
							QuestNode.DeveloperName = FName(*NewText.ToString());
						});
				});
		}

		if (ColumnName == LxQuestNodeTableColumns::DisplayName)
		{
			return SNew(SEditableTextBox)
				.Text_Lambda([this]()
				{
					const FLxQuestNodeDefinition* QuestNode = FindQuestNode();
					return QuestNode ? QuestNode->DisplayName : FText::GetEmpty();
				})
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
				{
					EditQuestNode(LOCTEXT("EditDisplayName", "修改任务节点可视化名称"),
						[&NewText](FLxQuestNodeDefinition& QuestNode)
						{
							QuestNode.DisplayName = NewText;
						});
				});
		}

		if (ColumnName == LxQuestNodeTableColumns::Description)
		{
			return SNew(SEditableTextBox)
				.Text_Lambda([this]()
				{
					const FLxQuestNodeDefinition* QuestNode = FindQuestNode();
					return QuestNode ? QuestNode->DisplayDescription : FText::GetEmpty();
				})
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
				{
					EditQuestNode(LOCTEXT("EditDescription", "修改任务节点可视化描述"),
						[&NewText](FLxQuestNodeDefinition& QuestNode)
						{
							QuestNode.DisplayDescription = NewText;
						});
				});
		}

		return SNew(SEditableTextBox)
			.Text_Lambda([this]()
			{
				const FLxQuestNodeDefinition* QuestNode = FindQuestNode();
				return QuestNode ? QuestNode->QuestContent : FText::GetEmpty();
			})
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
			{
				EditQuestNode(LOCTEXT("EditQuestContent", "修改任务节点任务内容"),
					[&NewText](FLxQuestNodeDefinition& QuestNode)
					{
						QuestNode.QuestContent = NewText;
					});
			});
	}

private:
	/** 查询当前行引用的任务节点定义。 */
	const FLxQuestNodeDefinition* FindQuestNode() const
	{
		return QuestSeriesAsset.IsValid() && Item.IsValid()
			? QuestSeriesAsset->FindQuestNodeByEditorId(Item->EditorNodeId)
			: nullptr;
	}

	/** 创建只显示“任务”标签树的单选下拉控件。 */
	TSharedRef<SWidget> CreateQuestIdPicker()
	{
		const FLxQuestNodeDefinition* QuestNode = FindQuestNode();
		EditedQuestId = MakeShared<FGameplayTag>(QuestNode ? QuestNode->QuestId : FGameplayTag());
		const FString FilterString = QuestSeriesAsset.IsValid() && QuestSeriesAsset->GetQuestSeriesId().IsValid()
			? QuestSeriesAsset->GetQuestSeriesId().ToString()
			: TEXT("任务");
		return SNew(SBox)
			.WidthOverride(440.0f)
			.HeightOverride(360.0f)
			[
				IGameplayTagsEditorModule::Get().MakeGameplayTagWidget(
					FOnSetGameplayTag::CreateSP(this, &SLxQuestNodeTableRow::SetQuestId),
					EditedQuestId,
					FilterString)
			];
	}

	/** 将下拉选择的任务标签写回节点，并拒绝根标签和重复ID。 */
	void SetQuestId(const FGameplayTag& NewQuestId)
	{
		if (!QuestSeriesAsset.IsValid() || !Item.IsValid())
		{
			return;
		}

		if (NewQuestId.IsValid() && !QuestSeriesAsset->IsQuestIdInSeries(NewQuestId))
		{
			return;
		}

		const bool bIsDuplicate = NewQuestId.IsValid() && QuestSeriesAsset->GetQuestNodes().ContainsByPredicate(
			[this, &NewQuestId](const FLxQuestNodeDefinition& Candidate)
			{
				return Candidate.EditorNodeId != Item->EditorNodeId && Candidate.QuestId == NewQuestId;
			});
		if (bIsDuplicate)
		{
			return;
		}

		EditQuestNode(LOCTEXT("EditQuestId", "修改任务ID"),
			[&NewQuestId](FLxQuestNodeDefinition& QuestNode)
			{
				QuestNode.QuestId = NewQuestId;
			});
	}

	/** 在事务中修改当前任务节点并通知编辑器刷新。 */
	void EditQuestNode(const FText& TransactionText, TFunctionRef<void(FLxQuestNodeDefinition&)> EditFunction)
	{
		if (!QuestSeriesAsset.IsValid() || !Item.IsValid())
		{
			return;
		}

		FLxQuestNodeDefinition* QuestNode = QuestSeriesAsset->EditQuestNodes().FindByPredicate(
			[this](const FLxQuestNodeDefinition& Candidate)
			{
				return Candidate.EditorNodeId == Item->EditorNodeId;
			});
		if (!QuestNode)
		{
			return;
		}

		const FScopedTransaction Transaction(TransactionText);
		QuestSeriesAsset->Modify();
		EditFunction(*QuestNode);
		QuestSeriesAsset->MarkPackageDirty();
		OnNodeChanged.ExecuteIfBound();
	}

	/** 当前表格行对应的任务节点ID。 */
	TSharedPtr<FLxQuestNodeListItem> Item;

	/** 当前行所属的任务系列资产。 */
	TWeakObjectPtr<ULxQuestSeriesAsset> QuestSeriesAsset;

	/** 当前节点被编辑后的刷新回调。 */
	FLxOnQuestNodeTableChanged OnNodeChanged;

	/** 当前打开的任务ID下拉控件所编辑的标签值。 */
	TSharedPtr<FGameplayTag> EditedQuestId;
};

void SLxQuestNodeTable::Construct(const FArguments& InArgs)
{
	QuestSeriesAsset = InArgs._QuestSeriesAsset;
	OnNodesChanged = InArgs._OnNodesChanged;
	RefreshItems();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("QuestSeriesIdLabel", "任务系列ID"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SComboButton)
				.ContentPadding(FMargin(6.0f, 2.0f))
				.OnGetMenuContent(this, &SLxQuestNodeTable::CreateQuestSeriesIdPicker)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return QuestSeriesAsset.IsValid() && QuestSeriesAsset->GetQuestSeriesId().IsValid()
							? FText::FromName(QuestSeriesAsset->GetQuestSeriesId().GetTagName())
							: LOCTEXT("SelectQuestSeriesId", "选择任务系列ID");
					})
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddQuestNode", "添加任务节点"))
				.ToolTipText(LOCTEXT("AddQuestNodeTooltip", "向当前任务系列添加一个尚未放置到关系图的任务节点"))
				.OnClicked(this, &SLxQuestNodeTable::AddQuestNode)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("DeleteQuestNode", "删除选中节点"))
				.IsEnabled(this, &SLxQuestNodeTable::CanDeleteSelectedQuestNode)
				.OnClicked(this, &SLxQuestNodeTable::DeleteSelectedQuestNode)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FLxQuestNodeListItem>>)
			.ListItemsSource(&Items)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &SLxQuestNodeTable::GenerateRow)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(LxQuestNodeTableColumns::QuestId)
				.DefaultLabel(LOCTEXT("QuestIdColumn", "任务ID"))
				.FillWidth(1.4f)
				+ SHeaderRow::Column(LxQuestNodeTableColumns::DeveloperName)
				.DefaultLabel(LOCTEXT("DeveloperNameColumn", "开发名称"))
				.FillWidth(0.8f)
				+ SHeaderRow::Column(LxQuestNodeTableColumns::DisplayName)
				.DefaultLabel(LOCTEXT("DisplayNameColumn", "可视化名称"))
				.FillWidth(1.0f)
				+ SHeaderRow::Column(LxQuestNodeTableColumns::Description)
				.DefaultLabel(LOCTEXT("DescriptionColumn", "可视化描述"))
				.FillWidth(1.5f)
				+ SHeaderRow::Column(LxQuestNodeTableColumns::QuestContent)
				.DefaultLabel(LOCTEXT("QuestContentColumn", "任务内容"))
				.FillWidth(1.5f)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 6.0f, 4.0f, 0.0f)
		[
			SNew(SExpandableArea)
			.InitiallyCollapsed(true)
			.HeaderContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("QuestIdManagerTitle", "任务ID标签管理"))
			]
			.BodyContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("QuestIdManagerHint", "任务系列与任务共用“任务”标签树。先创建并选择系列标签，再在它下面创建具体任务标签。"))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SGameplayTagPicker)
					.Filter(TEXT("任务"))
					.SettingsName(TEXT("LxQuestIdManager"))
					.ReadOnly(false)
					.MultiSelect(false)
					.GameplayTagPickerMode(EGameplayTagPickerMode::ManagementMode)
					.MaxHeight(240.0f)
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("GraphHint", "在右侧画布空白处右键，只能选择当前系列中尚未放置的任务节点。"))
			.AutoWrapText(true)
		]
	];
}

TSharedRef<SWidget> SLxQuestNodeTable::CreateQuestSeriesIdPicker()
{
	EditedQuestSeriesId = MakeShared<FGameplayTag>(QuestSeriesAsset.IsValid()
		? QuestSeriesAsset->GetQuestSeriesId()
		: FGameplayTag());
	return SNew(SBox)
		.WidthOverride(440.0f)
		.HeightOverride(360.0f)
		[
			IGameplayTagsEditorModule::Get().MakeGameplayTagWidget(
				FOnSetGameplayTag::CreateSP(this, &SLxQuestNodeTable::SetQuestSeriesId),
				EditedQuestSeriesId,
				TEXT("任务"))
		];
}

void SLxQuestNodeTable::SetQuestSeriesId(const FGameplayTag& NewQuestSeriesId)
{
	if (!QuestSeriesAsset.IsValid())
	{
		return;
	}

	const FString QuestSeriesIdString = NewQuestSeriesId.ToString();
	if (NewQuestSeriesId.IsValid() && !QuestSeriesIdString.StartsWith(TEXT("任务.")))
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("EditQuestSeriesId", "修改任务系列ID"));
	QuestSeriesAsset->Modify();
	QuestSeriesAsset->SetQuestSeriesId(NewQuestSeriesId);
	QuestSeriesAsset->MarkPackageDirty();
	RefreshItems();
	OnNodesChanged.ExecuteIfBound();
}

void SLxQuestNodeTable::RefreshItems()
{
	Items.Reset();
	if (QuestSeriesAsset.IsValid())
	{
		for (const FLxQuestNodeDefinition& QuestNode : QuestSeriesAsset->GetQuestNodes())
		{
			TSharedPtr<FLxQuestNodeListItem> Item = MakeShared<FLxQuestNodeListItem>();
			Item->EditorNodeId = QuestNode.EditorNodeId;
			Items.Add(Item);
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SLxQuestNodeTable::GenerateRow(TSharedPtr<FLxQuestNodeListItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLxQuestNodeTableRow, OwnerTable)
		.Item(Item)
		.QuestSeriesAsset(QuestSeriesAsset.Get())
		.OnNodeChanged(OnNodesChanged);
}

FReply SLxQuestNodeTable::AddQuestNode()
{
	if (!QuestSeriesAsset.IsValid())
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddQuestNodeTransaction", "添加任务节点"));
	QuestSeriesAsset->Modify();

	FLxQuestNodeDefinition& NewQuestNode = QuestSeriesAsset->EditQuestNodes().AddDefaulted_GetRef();
	NewQuestNode.EditorNodeId = FGuid::NewGuid();
	NewQuestNode.DeveloperName = FName(*FString::Printf(TEXT("QuestNode_%d"), QuestSeriesAsset->GetQuestNodes().Num()));
	NewQuestNode.DisplayName = LOCTEXT("NewQuestNodeName", "新任务节点");
	QuestSeriesAsset->MarkPackageDirty();

	RefreshItems();
	OnNodesChanged.ExecuteIfBound();
	if (ListView.IsValid() && Items.Num() > 0)
	{
		ListView->SetSelection(Items.Last());
	}
	return FReply::Handled();
}

FReply SLxQuestNodeTable::DeleteSelectedQuestNode()
{
	if (!QuestSeriesAsset.IsValid() || !ListView.IsValid())
	{
		return FReply::Handled();
	}

	const TArray<TSharedPtr<FLxQuestNodeListItem>> SelectedItems = ListView->GetSelectedItems();
	if (SelectedItems.IsEmpty() || !SelectedItems[0].IsValid())
	{
		return FReply::Handled();
	}

	const FGuid EditorNodeId = SelectedItems[0]->EditorNodeId;
	const FScopedTransaction Transaction(LOCTEXT("DeleteQuestNodeTransaction", "删除任务节点"));
	QuestSeriesAsset->Modify();
	QuestSeriesAsset->EditQuestNodes().RemoveAll([&EditorNodeId](const FLxQuestNodeDefinition& QuestNode)
	{
		return QuestNode.EditorNodeId == EditorNodeId;
	});
	QuestSeriesAsset->MarkPackageDirty();

	RefreshItems();
	OnNodesChanged.ExecuteIfBound();
	return FReply::Handled();
}

bool SLxQuestNodeTable::CanDeleteSelectedQuestNode() const
{
	return ListView.IsValid() && ListView->GetNumItemsSelected() > 0;
}

#undef LOCTEXT_NAMESPACE
