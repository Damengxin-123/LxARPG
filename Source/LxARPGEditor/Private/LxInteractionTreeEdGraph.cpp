#include "LxInteractionTreeEdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

ULxInteractionTreeAsset* ULxInteractionTreeEdGraph::GetAsset() const
{
	return Cast<ULxInteractionTreeAsset>(GetOuter());
}

void ULxInteractionTreeEdGraph::EnsureStartNode()
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (const ULxInteractionTreeEdGraphNode* TreeNode = Cast<ULxInteractionTreeEdGraphNode>(Node); TreeNode && TreeNode->bStart) return;
	}
	Modify();
	FGraphNodeCreator<ULxInteractionTreeEdGraphNode> Creator(*this);
	ULxInteractionTreeEdGraphNode* Start = Creator.CreateNode();
	Start->bStart = true;
	Start->NodePosX = 0;
	Start->NodePosY = 0;
	Creator.Finalize();
}

void ULxInteractionTreeEdGraph::SynchronizeAsset()
{
	ULxInteractionTreeAsset* Asset = GetAsset();
	if (!Asset) return;
	Asset->Modify();
	Asset->Nodes.Reset();
	Asset->Roots.Reset();
	for (UEdGraphNode* Node : Nodes)
	{
		ULxInteractionTreeEdGraphNode* TreeNode = Cast<ULxInteractionTreeEdGraphNode>(Node);
		if (!TreeNode) continue;
		if (TreeNode->Data)
		{
			TreeNode->Data->Modify();
			TreeNode->Data->Children.Reset();
			Asset->Nodes.Add(TreeNode->Data);
		}
		TArray<ULxInteractionTreeNodeData*> Children;
		for (const UEdGraphPin* Pin : TreeNode->Pins)
		{
			if (Pin->Direction != EGPD_Output) continue;
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				const ULxInteractionTreeEdGraphNode* Child = Cast<ULxInteractionTreeEdGraphNode>(Linked->GetOwningNode());
				if (Child && Child->Data) Children.Add(Child->Data);
			}
		}
		// 显式排序值决定选项顺序，标识打破平局；拖动画布不改变网络节点序号。
		Children.Sort([](const ULxInteractionTreeNodeData& A, const ULxInteractionTreeNodeData& B)
		{
			return A.OptionOrder != B.OptionOrder ? A.OptionOrder < B.OptionOrder : A.NodeId < B.NodeId;
		});
		for (const ULxInteractionTreeNodeData* Child : Children)
		{
			if (TreeNode->bStart) Asset->Roots.Add(Child->NodeId);
			else if (TreeNode->Data) TreeNode->Data->Children.Add(Child->NodeId);
		}
	}
	Asset->MarkPackageDirty();
}

void ULxInteractionTreeEdGraphNode::AllocateDefaultPins()
{
	if (!bStart) CreatePin(EGPD_Input, TEXT("交互流程"), TEXT("父项"));
	if (bStart || !Data || Data->Type != ELxInteractionActionType::InteractionExit)
		CreatePin(EGPD_Output, TEXT("交互流程"), TEXT("子项"));
}

FText ULxInteractionTreeEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (bStart) return NSLOCTEXT("交互树", "开始", "开始");
	if (!Data) return NSLOCTEXT("交互树", "无效", "无效节点");
	FText Prompt = Data->PromptText;
	if (Data->Type == ELxInteractionActionType::Quest && Data->QuestConfig.bUseQuestDisplayText)
	{
		Prompt = FText::Format(NSLOCTEXT("交互树", "任务名称预览", "使用任务名称：{0}"), FText::FromName(Data->QuestConfig.QuestId.GetTagName()));
	}
	if (Prompt.IsEmpty()) Prompt = NSLOCTEXT("交互树", "未填写", "未填写交互提示");
	return FText::Format(NSLOCTEXT("交互树", "标题", "{0}\n{1}"), ULxInteractionTreeAsset::GetTypeLabel(Data->Type), Prompt);
}

FLinearColor ULxInteractionTreeEdGraphNode::GetNodeTitleColor() const
{
	if (bStart) return FLinearColor(0.1f, 0.55f, 0.25f);
	if (Data && ULxInteractionTreeAsset::IsFunctionType(Data->Type)) return FLinearColor(0.55f, 0.25f, 0.08f);
	return FLinearColor(0.08f, 0.3f, 0.55f);
}

void ULxInteractionTreeEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin) return;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction != FromPin->Direction)
		{
			GetSchema()->TryCreateConnection(FromPin, Pin);
			return;
		}
	}
}

UEdGraphNode* FLxInteractionTreeNewNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2f& Location, bool bSelectNewNode)
{
	ULxInteractionTreeEdGraph* Graph = Cast<ULxInteractionTreeEdGraph>(ParentGraph);
	ULxInteractionTreeAsset* Asset = Graph ? Graph->GetAsset() : nullptr;
	if (!Asset || !Asset->CanAddNode(Type)) return nullptr;
	const FScopedTransaction Transaction(NSLOCTEXT("交互树", "新增", "添加交互节点"));
	Graph->Modify();
	Asset->Modify();
	ULxInteractionTreeNodeData* Data = NewObject<ULxInteractionTreeNodeData>(Asset, NAME_None, RF_Transactional);
	Data->Type = Type;
	Data->bCloseInteractionDialogue = ULxInteractionTreeAsset::IsFunctionType(Type);
	Data->NodeId = FGuid::NewGuid();
	Data->OptionOrder = Asset->Nodes.Num();
	Data->PromptText = ULxInteractionTreeAsset::GetTypeLabel(Type);
	Asset->Nodes.Add(Data);
	FGraphNodeCreator<ULxInteractionTreeEdGraphNode> Creator(*Graph);
	ULxInteractionTreeEdGraphNode* Node = Creator.CreateUserInvokedNode(bSelectNewNode);
	Node->Data = Data;
	Node->NodePosX = FMath::RoundToInt(Location.X);
	Node->NodePosY = FMath::RoundToInt(Location.Y);
	Creator.Finalize();
	Node->AutowireNewNode(FromPin);
	Graph->SynchronizeAsset();
	Graph->NotifyGraphChanged();
	return Node;
}

void ULxInteractionTreeEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& Builder) const
{
	const ULxInteractionTreeEdGraph* Graph = Cast<ULxInteractionTreeEdGraph>(Builder.CurrentGraph);
	const ULxInteractionTreeAsset* Asset = Graph ? Graph->GetAsset() : nullptr;
	if (!Asset) return;
	for (int32 Value = 0; Value < StaticEnum<ELxInteractionActionType>()->NumEnums() - 1; ++Value)
	{
		const ELxInteractionActionType Type = static_cast<ELxInteractionActionType>(StaticEnum<ELxInteractionActionType>()->GetValueByIndex(Value));
		if (!Asset->CanAddNode(Type)) continue;
		TSharedRef<FLxInteractionTreeNewNodeAction> Action = MakeShared<FLxInteractionTreeNewNodeAction>(
			ULxInteractionTreeAsset::IsFunctionType(Type) ? NSLOCTEXT("交互树", "功能", "已启用功能") : NSLOCTEXT("交互树", "普通", "普通节点"),
			ULxInteractionTreeAsset::GetTypeLabel(Type), FText(), 0);
		Action->Type = Type;
		Builder.AddAction(Action);
	}
}

const FPinConnectionResponse ULxInteractionTreeEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	const FPinConnectionResponse Invalid(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("交互树", "方向错误", "请连接不同节点的子项与父项引脚"));
	if (!A || !B || A->Direction == B->Direction || A->GetOwningNode() == B->GetOwningNode()
		|| A->GetOwningNode()->GetGraph() != B->GetOwningNode()->GetGraph()) return Invalid;
	const UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
	const UEdGraphPin* Input = A->Direction == EGPD_Input ? A : B;
	const ULxInteractionTreeEdGraphNode* Source = Cast<ULxInteractionTreeEdGraphNode>(Output->GetOwningNode());
	const ULxInteractionTreeEdGraphNode* Target = Cast<ULxInteractionTreeEdGraphNode>(Input->GetOwningNode());
	if (!Source || !Target || Target->bStart) return Invalid;
	if (!Input->LinkedTo.IsEmpty())
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("交互树", "单父项", "每个节点只能连接一个父项，请先断开原连接"));
	if (Target->Data && Target->Data->Type == ELxInteractionActionType::Entrance && !Source->bStart)
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("交互树", "入口位置", "入口只能连接开始节点"));
	TSet<const UEdGraphNode*> Visited;
	TArray<const UEdGraphNode*> Pending {Target};
	while (!Pending.IsEmpty())
	{
		const UEdGraphNode* Node = Pending.Pop();
		if (Node == Source) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NSLOCTEXT("交互树", "循环", "不能创建循环"));
		if (Visited.Contains(Node)) continue;
		Visited.Add(Node);
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output)
				for (const UEdGraphPin* Link : Pin->LinkedTo) Pending.Add(Link->GetOwningNode());
		}
	}
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, NSLOCTEXT("交互树", "连接", "连接子项"));
}

bool ULxInteractionTreeEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const bool bResult = Super::TryCreateConnection(A, B);
	if (bResult)
	{
		ULxInteractionTreeEdGraph* Graph = CastChecked<ULxInteractionTreeEdGraph>(A->GetOwningNode()->GetGraph());
		Graph->SynchronizeAsset();
		Graph->NotifyGraphChanged();
	}
	return bResult;
}

void ULxInteractionTreeEdGraphSchema::BreakNodeLinks(UEdGraphNode& Node) const
{
	Super::BreakNodeLinks(Node);
	if (ULxInteractionTreeEdGraph* Graph = Cast<ULxInteractionTreeEdGraph>(Node.GetGraph()))
	{
		Graph->SynchronizeAsset(); Graph->NotifyGraphChanged();
	}
}

void ULxInteractionTreeEdGraphSchema::BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const
{
	Super::BreakPinLinks(Pin, bNotify);
	if (ULxInteractionTreeEdGraph* Graph = Cast<ULxInteractionTreeEdGraph>(Pin.GetOwningNode()->GetGraph()))
	{
		Graph->SynchronizeAsset(); Graph->NotifyGraphChanged();
	}
}

void ULxInteractionTreeEdGraphSchema::BreakSinglePinLink(UEdGraphPin* A, UEdGraphPin* B) const
{
	Super::BreakSinglePinLink(A, B);
	if (A)
	{
		if (ULxInteractionTreeEdGraph* Graph = Cast<ULxInteractionTreeEdGraph>(A->GetOwningNode()->GetGraph()))
		{
			Graph->SynchronizeAsset(); Graph->NotifyGraphChanged();
		}
	}
}
