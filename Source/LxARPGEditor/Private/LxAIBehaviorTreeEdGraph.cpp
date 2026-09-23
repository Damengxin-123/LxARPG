#include "LxAIBehaviorTreeEdGraph.h"

#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

namespace
{
/** 返回节点的输出引脚。 */
UEdGraphPin* FindOutputPin(ULxAIBehaviorTreeEdGraphNode* Node)
{
	if (!Node) return nullptr;
	for (UEdGraphPin* Pin : Node->Pins)
		if (Pin && Pin->Direction == EGPD_Output) return Pin;
	return nullptr;
}

/** 返回节点的输入引脚。 */
UEdGraphPin* FindInputPin(ULxAIBehaviorTreeEdGraphNode* Node)
{
	if (!Node) return nullptr;
	for (UEdGraphPin* Pin : Node->Pins)
		if (Pin && Pin->Direction == EGPD_Input) return Pin;
	return nullptr;
}

/** 为默认模板创建一个配置节点。 */
ULxAIBehaviorTreeEdGraphNode* CreateTemplateNode(ULxAIBehaviorTreeEdGraph& Graph, ULxAIBehaviorTreeAsset& Asset,
	ELxAIBehaviorNodeKind Kind, ELxAIBehaviorState State, ELxAIBehaviorAction Action, int32 Order, int32 X, int32 Y)
{
	ULxAIBehaviorTreeNodeData* Data = NewObject<ULxAIBehaviorTreeNodeData>(&Asset, NAME_None, RF_Transactional);
	Data->Kind = Kind;
	Data->State = State;
	Data->Action = Action;
	Data->Order = Order;
	Data->NodeId = FGuid::NewGuid();
	FGraphNodeCreator<ULxAIBehaviorTreeEdGraphNode> Creator(Graph);
	ULxAIBehaviorTreeEdGraphNode* Node = Creator.CreateNode();
	Node->Data = Data;
	Node->NodePosX = X;
	Node->NodePosY = Y;
	Creator.Finalize();
	return Node;
}

/** 将父节点输出连接到子节点输入。 */
void LinkTemplateNodes(ULxAIBehaviorTreeEdGraphNode* Parent, ULxAIBehaviorTreeEdGraphNode* Child)
{
	UEdGraphPin* Output = FindOutputPin(Parent);
	UEdGraphPin* Input = FindInputPin(Child);
	if (Output && Input) Output->MakeLinkTo(Input);
}

/** 向菜单添加一个指定配置节点。 */
void AddMenuAction(FGraphContextMenuBuilder& Builder, ELxAIBehaviorNodeKind Kind, ELxAIBehaviorState State,
	ELxAIBehaviorAction Action, const FText& Category, const FText& Label, const FText& Tooltip)
{
	TSharedRef<FLxAIBehaviorTreeNewNodeAction> NewAction = MakeShared<FLxAIBehaviorTreeNewNodeAction>(Category, Label, Tooltip, 0);
	NewAction->Kind = Kind;
	NewAction->State = State;
	NewAction->Action = Action;
	Builder.AddAction(NewAction);
}
}

ULxAIBehaviorTreeAsset* ULxAIBehaviorTreeEdGraph::GetAsset() const
{
	return Cast<ULxAIBehaviorTreeAsset>(GetOuter());
}

void ULxAIBehaviorTreeEdGraph::EnsureEntryNodes()
{
	ULxAIBehaviorTreeAsset* Asset = GetAsset();
	if (!Asset) return;
	// PostLoad 先升级数据模型；此处重建缺失的可视入口，保留原状态、阶段、行为节点和位置。
	Asset->UpgradeLegacyEntries();
	if (Nodes.IsEmpty() && !Asset->Nodes.IsEmpty())
	{
		// 无编辑图的旧数据资产只含派生序列；重建图时必须先保全全部配置对象。
		Modify();
		TMap<FGuid, ULxAIBehaviorTreeEdGraphNode*> Rebuilt;
		for (ULxAIBehaviorTreeNodeData* Data : Asset->Nodes)
		{
			if (!Data) continue;
			FGraphNodeCreator<ULxAIBehaviorTreeEdGraphNode> Creator(*this);
			ULxAIBehaviorTreeEdGraphNode* Node = Creator.CreateNode();
			Node->Data = Data;
			Node->NodePosX = Data->Kind == ELxAIBehaviorNodeKind::Entry ? 0
				: Data->Kind == ELxAIBehaviorNodeKind::State ? 350
				: Data->Kind == ELxAIBehaviorNodeKind::Phase ? 700 : 1050;
			Node->NodePosY = Data->Kind == ELxAIBehaviorNodeKind::Entry ? static_cast<int32>(Data->Entry) * 500
				: static_cast<int32>(Data->GetState()) * 350 + (Data->Kind == ELxAIBehaviorNodeKind::Action ? Data->Order * 180 : 0);
			Creator.Finalize();
			Rebuilt.Add(Data->NodeId, Node);
		}
		for (ULxAIBehaviorTreeNodeData* Parent : Asset->Nodes)
		{
			ULxAIBehaviorTreeEdGraphNode** ParentNode = Parent ? Rebuilt.Find(Parent->NodeId) : nullptr;
			if (!ParentNode) continue;
			for (const FGuid& ChildId : Parent->Children)
				if (ULxAIBehaviorTreeEdGraphNode** ChildNode = Rebuilt.Find(ChildId)) LinkTemplateNodes(*ParentNode, *ChildNode);
		}
	}
	TMap<FGuid, ULxAIBehaviorTreeEdGraphNode*> ById;
	TArray<ULxAIBehaviorTreeEdGraphNode*> OldStarts;
	TMap<ELxAIBehaviorEntry, ULxAIBehaviorTreeEdGraphNode*> Entries;
	for (UEdGraphNode* RawNode : Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (!Node) continue;
		if (Node->bStart) OldStarts.Add(Node);
		if (Node->Data)
		{
			ById.Add(Node->Data->NodeId, Node);
			if (Node->Data->Kind == ELxAIBehaviorNodeKind::Entry) Entries.Add(Node->Data->Entry, Node);
		}
	}
	static const ELxAIBehaviorEntry EntryOrder[] = {ELxAIBehaviorEntry::Calm, ELxAIBehaviorEntry::EnemyFound,
		ELxAIBehaviorEntry::EnemyNear, ELxAIBehaviorEntry::Attacked};
	bool bChanged = false;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(EntryOrder); ++Index)
	{
		const ELxAIBehaviorEntry Entry = EntryOrder[Index];
		if (Entries.Contains(Entry)) continue;
		ULxAIBehaviorTreeNodeData* Data = nullptr;
		for (ULxAIBehaviorTreeNodeData* Candidate : Asset->Nodes)
			if (Candidate && Candidate->Kind == ELxAIBehaviorNodeKind::Entry && Candidate->Entry == Entry) { Data = Candidate; break; }
		if (!Data)
		{
			Data = NewObject<ULxAIBehaviorTreeNodeData>(Asset, NAME_None, RF_Transactional);
			Data->Kind = ELxAIBehaviorNodeKind::Entry;
			Data->Entry = Entry;
			Data->EntryPriority = ULxAIBehaviorTreeAsset::GetDefaultEntryPriority(Entry);
			Data->NodeId = FGuid::NewGuid();
			Asset->Modify();
			Asset->Nodes.Add(Data);
			Asset->Roots.Add(Data->NodeId);
		}
		Modify();
		FGraphNodeCreator<ULxAIBehaviorTreeEdGraphNode> Creator(*this);
		ULxAIBehaviorTreeEdGraphNode* Node = Creator.CreateNode();
		Node->Data = Data;
		Node->NodePosX = 0;
		Node->NodePosY = Index * 500;
		Creator.Finalize();
		Entries.Add(Entry, Node);
		bChanged = true;
	}
	// 数据升级决定旧状态该接入哪些事件；图中已有分支不重建、不重新定位。
	for (const auto& Pair : Entries)
	{
		ULxAIBehaviorTreeEdGraphNode* EntryNode = Pair.Value;
		for (const FGuid& ChildId : EntryNode->Data->Children)
		{
			ULxAIBehaviorTreeEdGraphNode** Child = ById.Find(ChildId);
			if (Child && FindOutputPin(EntryNode) && FindInputPin(*Child)
				&& !FindOutputPin(EntryNode)->LinkedTo.Contains(FindInputPin(*Child)))
			{
				LinkTemplateNodes(EntryNode, *Child);
				bChanged = true;
			}
		}
	}
	for (ULxAIBehaviorTreeEdGraphNode* Start : OldStarts)
	{
		Modify();
		Start->DestroyNode();
		bChanged = true;
	}
	if (bChanged) { SynchronizeAsset(); NotifyGraphChanged(); }
}

void ULxAIBehaviorTreeEdGraph::InitializeDefaultTree()
{
	// 允许全空或仅有固定入口的图；任何业务节点都使模板初始化成为无操作。
	for (UEdGraphNode* Node : Nodes)
	{
		const ULxAIBehaviorTreeEdGraphNode* TreeNode = Cast<ULxAIBehaviorTreeEdGraphNode>(Node);
		if (!TreeNode || !TreeNode->Data || TreeNode->Data->Kind != ELxAIBehaviorNodeKind::Entry) return;
	}
	ULxAIBehaviorTreeAsset* Asset = GetAsset();
	if (!Asset) return;
	for (const ULxAIBehaviorTreeNodeData* Data : Asset->Nodes)
		if (!Data || Data->Kind != ELxAIBehaviorNodeKind::Entry) return;
	Modify();
	Asset->Modify();
	EnsureEntryNodes();
	TMap<ELxAIBehaviorEntry, ULxAIBehaviorTreeEdGraphNode*> Entries;
	for (UEdGraphNode* RawNode : Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* Candidate = Cast<ULxAIBehaviorTreeEdGraphNode>(RawNode);
		if (Candidate && Candidate->Data && Candidate->Data->Kind == ELxAIBehaviorNodeKind::Entry)
			Entries.Add(Candidate->Data->Entry, Candidate);
	}

	/** 默认模板中每种状态的布局与行为集合。 */
	struct FStateTemplate
	{
		/** 该组的状态分类。 */
		ELxAIBehaviorState State;
		/** 状态及阶段节点在图空间中的纵向位置。 */
		int32 CenterY;
		/** 按排列顺序创建的具体行为。 */
		TArray<ELxAIBehaviorAction> Actions;
	};
	const TArray<FStateTemplate> Templates = {
		{ELxAIBehaviorState::Idle, 0, {ELxAIBehaviorAction::Wait}},
		{ELxAIBehaviorState::Patrol, 270, {ELxAIBehaviorAction::PointPatrol, ELxAIBehaviorAction::RoutePatrol}},
		{ELxAIBehaviorState::Alert, 540, {ELxAIBehaviorAction::Alert}},
		{ELxAIBehaviorState::Combat, 990, {ELxAIBehaviorAction::MeleeSkill, ELxAIBehaviorAction::RangedSkill, ELxAIBehaviorAction::BuffSkill, ELxAIBehaviorAction::Defend}},
		{ELxAIBehaviorState::Flee, 1530, {ELxAIBehaviorAction::RandomFlee, ELxAIBehaviorAction::PointFlee, ELxAIBehaviorAction::RouteFlee}}
	};
	for (int32 StateIndex = 0; StateIndex < Templates.Num(); ++StateIndex)
	{
		const FStateTemplate& Template = Templates[StateIndex];
		ULxAIBehaviorTreeEdGraphNode* StateNode = CreateTemplateNode(*this, *Asset, ELxAIBehaviorNodeKind::State,
			Template.State, ELxAIBehaviorAction::Wait, StateIndex, 350, Template.CenterY);
		ULxAIBehaviorTreeEdGraphNode* PhaseNode = CreateTemplateNode(*this, *Asset, ELxAIBehaviorNodeKind::Phase,
			Template.State, ELxAIBehaviorAction::Wait, 0, 700, Template.CenterY);
		for (ELxAIBehaviorEntry Entry : ULxAIBehaviorTreeAsset::GetDefaultEntriesForState(Template.State))
			if (ULxAIBehaviorTreeEdGraphNode** EntryNode = Entries.Find(Entry)) LinkTemplateNodes(*EntryNode, StateNode);
		LinkTemplateNodes(StateNode, PhaseNode);
		const int32 FirstActionY = Template.CenterY - ((Template.Actions.Num() - 1) * 180) / 2;
		for (int32 ActionIndex = 0; ActionIndex < Template.Actions.Num(); ++ActionIndex)
		{
			const ELxAIBehaviorAction Action = Template.Actions[ActionIndex];
			ULxAIBehaviorTreeEdGraphNode* ActionNode = CreateTemplateNode(*this, *Asset, ELxAIBehaviorNodeKind::Action,
				Template.State, Action, ActionIndex, 1050, FirstActionY + ActionIndex * 180);
			LinkTemplateNodes(PhaseNode, ActionNode);
		}
	}
	SynchronizeAsset();
	NotifyGraphChanged();
}

void ULxAIBehaviorTreeEdGraph::SynchronizeAsset()
{
	ULxAIBehaviorTreeAsset* Asset = GetAsset();
	if (!Asset) return;
	Asset->Modify();
	Asset->Nodes.Reset();
	Asset->Roots.Reset();
	for (UEdGraphNode* Node : Nodes)
	{
		ULxAIBehaviorTreeEdGraphNode* TreeNode = Cast<ULxAIBehaviorTreeEdGraphNode>(Node);
		if (!TreeNode) continue;
		if (TreeNode->Data)
		{
			TreeNode->Data->Modify();
			TreeNode->Data->Children.Reset();
			Asset->Nodes.Add(TreeNode->Data);
		}
		TArray<ULxAIBehaviorTreeNodeData*> Children;
		for (const UEdGraphPin* Pin : TreeNode->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output) continue;
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				const ULxAIBehaviorTreeEdGraphNode* Child = Linked ? Cast<ULxAIBehaviorTreeEdGraphNode>(Linked->GetOwningNode()) : nullptr;
				if (Child && Child->Data) Children.Add(Child->Data);
			}
		}
		Children.Sort([](const ULxAIBehaviorTreeNodeData& A, const ULxAIBehaviorTreeNodeData& B)
		{
			return A.Order != B.Order ? A.Order < B.Order : A.NodeId < B.NodeId;
		});
		for (const ULxAIBehaviorTreeNodeData* Child : Children)
		{
			if (TreeNode->Data) TreeNode->Data->Children.Add(Child->NodeId);
		}
	}
	for (const auto Entry : {ELxAIBehaviorEntry::Calm, ELxAIBehaviorEntry::EnemyFound, ELxAIBehaviorEntry::EnemyNear, ELxAIBehaviorEntry::Attacked})
		for (const ULxAIBehaviorTreeNodeData* Node : Asset->Nodes)
			if (Node && Node->Kind == ELxAIBehaviorNodeKind::Entry && Node->Entry == Entry) Asset->Roots.Add(Node->NodeId);
	Asset->Nodes.Sort([](const TObjectPtr<ULxAIBehaviorTreeNodeData>& APtr, const TObjectPtr<ULxAIBehaviorTreeNodeData>& BPtr)
	{
		const ULxAIBehaviorTreeNodeData& A = *APtr;
		const ULxAIBehaviorTreeNodeData& B = *BPtr;
		if (A.Kind != B.Kind) return static_cast<uint8>(A.Kind) < static_cast<uint8>(B.Kind);
		if (A.GetState() != B.GetState()) return static_cast<uint8>(A.GetState()) < static_cast<uint8>(B.GetState());
		return A.Order != B.Order ? A.Order < B.Order : A.NodeId < B.NodeId;
	});
	Asset->MarkPackageDirty();
}

void ULxAIBehaviorTreeEdGraphNode::AllocateDefaultPins()
{
	if (!bStart && (!Data || Data->Kind != ELxAIBehaviorNodeKind::Entry)) CreatePin(EGPD_Input, TEXT("AI行为流程"), TEXT("父项"));
	if (bStart || !Data || Data->Kind != ELxAIBehaviorNodeKind::Action)
		CreatePin(EGPD_Output, TEXT("AI行为流程"), TEXT("子项"));
}

FText ULxAIBehaviorTreeEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (bStart) return NSLOCTEXT("AI行为树编辑器", "开始标题", "开始");
	if (!Data) return NSLOCTEXT("AI行为树编辑器", "无效标题", "无效节点");
	if (Data->Kind == ELxAIBehaviorNodeKind::Entry)
		return FText::Format(NSLOCTEXT("AI行为树编辑器", "事件标题", "{0}\n优先级 {1}"),
			ULxAIBehaviorTreeAsset::GetEntryLabel(Data->Entry), FText::AsNumber(Data->EntryPriority));
	if (Data->Kind != ELxAIBehaviorNodeKind::Action)
	{
		return FText::Format(NSLOCTEXT("AI行为树编辑器", "条件标题", "{0}\n生命值 [{1}, {2}]"),
			Data->GetDisplayLabel(), FText::AsNumber(Data->HealthRange.Min), FText::AsNumber(Data->HealthRange.Max));
	}
	FText Summary;
	switch (Data->Action)
	{
	case ELxAIBehaviorAction::Wait: Summary = FText::Format(NSLOCTEXT("AI行为树编辑器", "等待摘要", "等待 {0} 秒"), FText::AsNumber(Data->WaitSeconds)); break;
	case ELxAIBehaviorAction::PointPatrol:
	case ELxAIBehaviorAction::PointFlee: Summary = Data->PointId.IsValid() ? FText::FromName(Data->PointId.GetTagName()) : NSLOCTEXT("AI行为树编辑器", "缺点位", "请补充点位ID"); break;
	case ELxAIBehaviorAction::RoutePatrol:
	case ELxAIBehaviorAction::RouteFlee: Summary = Data->RouteId.IsValid() ? FText::FromName(Data->RouteId.GetTagName()) : NSLOCTEXT("AI行为树编辑器", "缺路线", "请补充路线ID"); break;
	case ELxAIBehaviorAction::MeleeSkill:
	case ELxAIBehaviorAction::RangedSkill:
	case ELxAIBehaviorAction::BuffSkill: Summary = Data->SkillItemId.IsValid() ? FText::FromName(Data->SkillItemId.GetTagName()) : NSLOCTEXT("AI行为树编辑器", "缺技能", "请补充技能物品ID"); break;
	case ELxAIBehaviorAction::Alert:
	case ELxAIBehaviorAction::Defend: Summary = FText::Format(NSLOCTEXT("AI行为树编辑器", "距离摘要", "距离 {0}～{1} 米"), FText::AsNumber(Data->MinDistanceMeters), FText::AsNumber(Data->MaxDistanceMeters)); break;
	case ELxAIBehaviorAction::RandomFlee: Summary = FText::Format(NSLOCTEXT("AI行为树编辑器", "逃跑摘要", "单次 {0} 米"), FText::AsNumber(Data->FleeStepMeters)); break;
	default: break;
	}
	return Summary.IsEmpty() ? Data->GetDisplayLabel()
		: FText::Format(NSLOCTEXT("AI行为树编辑器", "行为标题", "{0}\n{1}"), Data->GetDisplayLabel(), Summary);
}

FLinearColor ULxAIBehaviorTreeEdGraphNode::GetNodeTitleColor() const
{
	if (bStart) return FLinearColor(0.1f, 0.55f, 0.25f);
	if (!Data) return FLinearColor(0.35f, 0.35f, 0.35f);
	if (Data->Kind == ELxAIBehaviorNodeKind::Entry) return FLinearColor(0.12f, 0.55f, 0.35f);
	FLinearColor StateColor;
	switch (Data->GetState())
	{
	case ELxAIBehaviorState::Idle: StateColor = FLinearColor(0.25f, 0.5f, 0.55f); break;
	case ELxAIBehaviorState::Patrol: StateColor = FLinearColor(0.15f, 0.55f, 0.3f); break;
	case ELxAIBehaviorState::Alert: StateColor = FLinearColor(0.8f, 0.55f, 0.08f); break;
	case ELxAIBehaviorState::Combat: StateColor = FLinearColor(0.75f, 0.15f, 0.12f); break;
	case ELxAIBehaviorState::Flee: StateColor = FLinearColor(0.65f, 0.25f, 0.1f); break;
	default: StateColor = FLinearColor(0.35f, 0.35f, 0.35f); break;
	}
	// 色相表示状态分类，亮度表示状态、阶段、行为层级。
	const float LayerScale = Data->Kind == ELxAIBehaviorNodeKind::State ? 1.15f
		: Data->Kind == ELxAIBehaviorNodeKind::Phase ? 0.82f : 1.0f;
	return FLinearColor(
		FMath::Clamp(StateColor.R * LayerScale, 0.0f, 1.0f),
		FMath::Clamp(StateColor.G * LayerScale, 0.0f, 1.0f),
		FMath::Clamp(StateColor.B * LayerScale, 0.0f, 1.0f));
}

FText ULxAIBehaviorTreeEdGraphNode::GetTooltipText() const
{
	if (bStart) return NSLOCTEXT("AI行为树编辑器", "开始提示", "旧版开始节点将在打开时迁移为四个分析入口。配置生成分析决策，具体行为执行尚未接入。");
	if (!Data) return NSLOCTEXT("AI行为树编辑器", "无效提示", "节点配置引用无效。");
	if (Data->Kind == ELxAIBehaviorNodeKind::Entry) return ULxAIBehaviorTreeAsset::GetEntryDescription(Data->Entry);
	if (Data->Kind == ELxAIBehaviorNodeKind::Action) return ULxAIBehaviorTreeAsset::GetActionDescription(Data->Action);
	return Data->Kind == ELxAIBehaviorNodeKind::State
		? NSLOCTEXT("AI行为树编辑器", "状态提示", "状态限定生命值闭区间，并包含同分类阶段。")
		: NSLOCTEXT("AI行为树编辑器", "阶段提示", "阶段生命值闭区间必须包含于父状态区间，并连接同分类行为。");
}

void ULxAIBehaviorTreeEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin) return;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction != FromPin->Direction)
		{
			GetSchema()->TryCreateConnection(FromPin, Pin);
			return;
		}
	}
}

UEdGraphNode* FLxAIBehaviorTreeNewNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2f& Location, bool bSelectNewNode)
{
	ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(ParentGraph);
	ULxAIBehaviorTreeAsset* Asset = Graph ? Graph->GetAsset() : nullptr;
	if (!Graph || !Asset || Kind == ELxAIBehaviorNodeKind::Entry
		|| (Kind == ELxAIBehaviorNodeKind::Action && ULxAIBehaviorTreeAsset::GetActionState(Action) != State)) return nullptr;
	const FScopedTransaction Transaction(NSLOCTEXT("AI行为树编辑器", "添加节点事务", "添加AI行为树节点"));
	Graph->Modify();
	Asset->Modify();
	ULxAIBehaviorTreeNodeData* Data = NewObject<ULxAIBehaviorTreeNodeData>(Asset, NAME_None, RF_Transactional);
	Data->Kind = Kind;
	Data->State = State;
	Data->Action = Action;
	Data->NodeId = FGuid::NewGuid();
	Data->Order = Asset->Nodes.Num();
	if (Kind == ELxAIBehaviorNodeKind::Phase && FromPin)
	{
		const ULxAIBehaviorTreeEdGraphNode* Parent = Cast<ULxAIBehaviorTreeEdGraphNode>(FromPin->GetOwningNode());
		if (Parent && Parent->Data && Parent->Data->Kind == ELxAIBehaviorNodeKind::State)
			Data->HealthRange = Parent->Data->HealthRange;
	}
	FGraphNodeCreator<ULxAIBehaviorTreeEdGraphNode> Creator(*Graph);
	ULxAIBehaviorTreeEdGraphNode* Node = Creator.CreateUserInvokedNode(bSelectNewNode);
	Node->Data = Data;
	Node->NodePosX = FMath::RoundToInt(Location.X);
	Node->NodePosY = FMath::RoundToInt(Location.Y);
	Creator.Finalize();
	Node->AutowireNewNode(FromPin);
	Graph->SynchronizeAsset();
	Graph->NotifyGraphChanged();
	return Node;
}

void ULxAIBehaviorTreeEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& Builder) const
{
	const ULxAIBehaviorTreeEdGraphNode* Source = Builder.FromPin && Builder.FromPin->Direction == EGPD_Output
		? Cast<ULxAIBehaviorTreeEdGraphNode>(Builder.FromPin->GetOwningNode()) : nullptr;
	const bool bBlankMenu = Builder.FromPin == nullptr;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const ELxAIBehaviorState State = static_cast<ELxAIBehaviorState>(Index);
		const FText StateLabel = ULxAIBehaviorTreeAsset::GetStateLabel(State);
		if (bBlankMenu || (Source && Source->Data && Source->Data->Kind == ELxAIBehaviorNodeKind::Entry))
			AddMenuAction(Builder, ELxAIBehaviorNodeKind::State, State, ELxAIBehaviorAction::Wait,
				NSLOCTEXT("AI行为树编辑器", "状态分类", "状态"), StateLabel, NSLOCTEXT("AI行为树编辑器", "状态菜单提示", "添加带生命值闭区间的状态节点。"));
		if (bBlankMenu || (Source && Source->Data && Source->Data->Kind == ELxAIBehaviorNodeKind::State && Source->Data->State == State))
			AddMenuAction(Builder, ELxAIBehaviorNodeKind::Phase, State, ELxAIBehaviorAction::Wait,
				FText::Format(NSLOCTEXT("AI行为树编辑器", "阶段分类", "阶段|{0}"), StateLabel),
				FText::Format(NSLOCTEXT("AI行为树编辑器", "阶段名称", "{0}阶段"), StateLabel), NSLOCTEXT("AI行为树编辑器", "阶段菜单提示", "添加同状态分类的阶段；从状态拖出时继承父状态生命区间。"));
	}
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const ELxAIBehaviorAction Action = static_cast<ELxAIBehaviorAction>(Index);
		const ELxAIBehaviorState State = ULxAIBehaviorTreeAsset::GetActionState(Action);
		if (!bBlankMenu && !(Source && Source->Data && Source->Data->Kind == ELxAIBehaviorNodeKind::Phase && Source->Data->State == State)) continue;
		AddMenuAction(Builder, ELxAIBehaviorNodeKind::Action, State, Action,
			FText::Format(NSLOCTEXT("AI行为树编辑器", "行为分类", "行为|{0}"), ULxAIBehaviorTreeAsset::GetStateLabel(State)),
			ULxAIBehaviorTreeAsset::GetActionLabel(Action), ULxAIBehaviorTreeAsset::GetActionDescription(Action));
	}
}

const FPinConnectionResponse ULxAIBehaviorTreeEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	const FPinConnectionResponse Invalid(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("AI行为树编辑器", "方向错误", "请连接不同节点的子项与父项引脚"));
	if (!A || !B || A->Direction == B->Direction || A->GetOwningNode() == B->GetOwningNode()
		|| A->GetOwningNode()->GetGraph() != B->GetOwningNode()->GetGraph()) return Invalid;
	const UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
	const UEdGraphPin* Input = A->Direction == EGPD_Input ? A : B;
	const ULxAIBehaviorTreeEdGraphNode* Parent = Cast<ULxAIBehaviorTreeEdGraphNode>(Output->GetOwningNode());
	const ULxAIBehaviorTreeEdGraphNode* Child = Cast<ULxAIBehaviorTreeEdGraphNode>(Input->GetOwningNode());
	if (!Parent || !Child || Parent->bStart || Child->bStart || !Parent->Data || !Child->Data) return Invalid;
	if (Output->LinkedTo.Contains(Input))
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("AI行为树编辑器", "重复边", "同一条连接不能重复创建"));
	if (Child->Data->Kind == ELxAIBehaviorNodeKind::Phase && !Input->LinkedTo.IsEmpty())
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("AI行为树编辑器", "阶段单父项", "阶段只能属于一个状态，请先断开原连接"));
	if (!ULxAIBehaviorTreeAsset::CanAttach(*Parent->Data, *Child->Data))
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("AI行为树编辑器", "层级归属", "只能连接 分析入口 → 状态 → 同类阶段 → 同类行为"));
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, NSLOCTEXT("AI行为树编辑器", "连接子项", "连接子项"));
}

bool ULxAIBehaviorTreeEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const bool bResult = Super::TryCreateConnection(A, B);
	if (bResult && A)
	{
		ULxAIBehaviorTreeEdGraph* Graph = CastChecked<ULxAIBehaviorTreeEdGraph>(A->GetOwningNode()->GetGraph());
		Graph->SynchronizeAsset();
		Graph->NotifyGraphChanged();
	}
	return bResult;
}

void ULxAIBehaviorTreeEdGraphSchema::BreakNodeLinks(UEdGraphNode& Node) const
{
	Super::BreakNodeLinks(Node);
	if (ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(Node.GetGraph())) { Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
}

void ULxAIBehaviorTreeEdGraphSchema::BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const
{
	Super::BreakPinLinks(Pin, bNotify);
	if (ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(Pin.GetOwningNode()->GetGraph())) { Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
}

void ULxAIBehaviorTreeEdGraphSchema::BreakSinglePinLink(UEdGraphPin* A, UEdGraphPin* B) const
{
	Super::BreakSinglePinLink(A, B);
	if (A)
		if (ULxAIBehaviorTreeEdGraph* Graph = Cast<ULxAIBehaviorTreeEdGraph>(A->GetOwningNode()->GetGraph())) { Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
}
