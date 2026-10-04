#include "LxSkillFlowEdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

ULxSkillFlowAsset* ULxSkillFlowEdGraph::GetAsset() const
{
	return Cast<ULxSkillFlowAsset>(GetOuter());
}

void ULxSkillFlowEdGraph::EnsureEntry()
{
	if (!Nodes.IsEmpty()) return;
	FLxSkillFlowNewNodeAction Action;
	Action.PerformAction(this, nullptr, FVector2f::ZeroVector, false);
}

void ULxSkillFlowEdGraph::SynchronizeAsset()
{
	ULxSkillFlowAsset* Asset = GetAsset();
	if (!Asset) return;
	Asset->Modify();
	Asset->Nodes.Reset();
	for (UEdGraphNode* Raw : Nodes)
	{
		ULxSkillFlowEdGraphNode* Node = Cast<ULxSkillFlowEdGraphNode>(Raw);
		if (!Node || !Node->Data) continue;
		Node->Data->Modify();
		Node->Data->Next.Reset(); Node->Data->Hit.Reset(); Node->Data->Finished.Reset();
		Asset->Nodes.Add(Node->Data);
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction != EGPD_Output) continue;
			TArray<FGuid>& Links = Pin->PinName == TEXT("命中") ? Node->Data->Hit
				: Pin->PinName == TEXT("结束") ? Node->Data->Finished : Node->Data->Next;
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				const ULxSkillFlowEdGraphNode* Target = Cast<ULxSkillFlowEdGraphNode>(Linked->GetOwningNode());
				if (Target && Target->Data) Links.AddUnique(Target->Data->Id);
			}
		}
	}
	Asset->MarkPackageDirty();
}

void ULxSkillFlowEdGraphNode::AllocateDefaultPins()
{
	if (!Data) return;
	if (Data->Kind == ELxSkillFlowNodeKind::Event) CreatePin(EGPD_Output, TEXT("技能流程"), TEXT("执行"));
	else
	{
		CreatePin(EGPD_Input, TEXT("技能流程"), TEXT("创建"));
		CreatePin(EGPD_Output, TEXT("技能流程"), TEXT("命中"));
		CreatePin(EGPD_Output, TEXT("技能流程"), TEXT("结束"));
	}
}

FText ULxSkillFlowEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!Data) return FText::FromString(TEXT("无效节点"));
	const FText Type = Data->Kind == ELxSkillFlowNodeKind::Event
		? StaticEnum<ELxSkillFlowEvent>()->GetDisplayNameTextByValue(static_cast<int64>(Data->Event))
		: StaticEnum<ELxSkillFlowNodeKind>()->GetDisplayNameTextByValue(static_cast<int64>(Data->Kind));
	const FText UnitName = Data->GetSkillUnitDisplayName();
	const FText Title = UnitName.IsEmpty() ? Type : UnitName;
	if (Data->Kind == ELxSkillFlowNodeKind::Event) return Title;
	const FText Lifetime = StaticEnum<ELxSkillFlowLifetime>()->GetDisplayNameTextByValue(static_cast<int64>(Data->Lifetime));
	return UnitName.IsEmpty()
		? FText::Format(NSLOCTEXT("技能流程", "默认单元标题", "{0}\n{1}"), Title, Lifetime)
		: FText::Format(NSLOCTEXT("技能流程", "具名单元标题", "{0}\n{1} · {2}"), Title, Type, Lifetime);
}

FLinearColor ULxSkillFlowEdGraphNode::GetNodeTitleColor() const
{
	if (!Data || Data->Kind == ELxSkillFlowNodeKind::Event) return FLinearColor(0.15f, 0.5f, 0.2f);
	return Data->Lifetime == ELxSkillFlowLifetime::Maintained ? FLinearColor(0.55f, 0.2f, 0.08f) : FLinearColor(0.1f, 0.3f, 0.6f);
}

void ULxSkillFlowEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin) return;
	for (UEdGraphPin* Pin : Pins)
		if (Pin->Direction != FromPin->Direction) { GetSchema()->TryCreateConnection(FromPin, Pin); return; }
}

UEdGraphNode* FLxSkillFlowNewNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode)
{
	ULxSkillFlowEdGraph* Graph = Cast<ULxSkillFlowEdGraph>(ParentGraph);
	if (!Graph || !Graph->GetAsset()) return nullptr;
	const FScopedTransaction Transaction(FText::FromString(TEXT("添加技能节点")));
	Graph->Modify(); Graph->GetAsset()->Modify();
	ULxSkillFlowNode* Data = NewObject<ULxSkillFlowNode>(Graph->GetAsset(), NAME_None, RF_Transactional);
	Data->Id = FGuid::NewGuid(); Data->Kind = Kind; Data->Event = Event;
	if (Kind == ELxSkillFlowNodeKind::Ray || Kind == ELxSkillFlowNodeKind::Melee) Data->Lifetime = ELxSkillFlowLifetime::Maintained;
	if (FromPin) Data->SpawnLocation = FromPin->PinName == TEXT("命中") ? ELxSkillUnitResultSpawnLocationType::HitLocation
		: FromPin->PinName == TEXT("结束") ? ELxSkillUnitResultSpawnLocationType::InvalidLocation : ELxSkillUnitResultSpawnLocationType::CasterLocation;
	FGraphNodeCreator<ULxSkillFlowEdGraphNode> Creator(*Graph);
	ULxSkillFlowEdGraphNode* Node = Creator.CreateUserInvokedNode(bSelectNewNode);
	Node->Data = Data; Node->NodePosX = FMath::RoundToInt(Location.X); Node->NodePosY = FMath::RoundToInt(Location.Y);
	Creator.Finalize();
	Node->AutowireNewNode(FromPin);
	Graph->SynchronizeAsset(); Graph->NotifyGraphChanged();
	return Node;
}

void ULxSkillFlowEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& Builder) const
{
	for (int32 Index = 0; Index < StaticEnum<ELxSkillFlowEvent>()->NumEnums() - 1; ++Index)
	{
		TSharedRef<FLxSkillFlowNewNodeAction> Action = MakeShared<FLxSkillFlowNewNodeAction>(FText::FromString(TEXT("角色事件")), StaticEnum<ELxSkillFlowEvent>()->GetDisplayNameTextByIndex(Index), FText(), 0);
		Action->Event = static_cast<ELxSkillFlowEvent>(Index);
		Builder.AddAction(Action);
	}
	for (int32 Index = 1; Index < StaticEnum<ELxSkillFlowNodeKind>()->NumEnums() - 1; ++Index)
	{
		TSharedRef<FLxSkillFlowNewNodeAction> Action = MakeShared<FLxSkillFlowNewNodeAction>(ULxSkillFlowNode::GetMenuCategory(static_cast<ELxSkillFlowNodeKind>(Index)), StaticEnum<ELxSkillFlowNodeKind>()->GetDisplayNameTextByIndex(Index), FText(), 0);
		Action->Kind = static_cast<ELxSkillFlowNodeKind>(Index);
		Builder.AddAction(Action);
	}
}

const FPinConnectionResponse ULxSkillFlowEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	const FPinConnectionResponse Invalid(CONNECT_RESPONSE_DISALLOW, FText::FromString(TEXT("请连接不同节点的事件出口与创建入口")));
	if (!A || !B || A->Direction == B->Direction || A->GetOwningNode() == B->GetOwningNode()
		|| A->GetOwningNode()->GetGraph() != B->GetOwningNode()->GetGraph() || A->LinkedTo.Contains(B)) return Invalid;
	const UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
	const UEdGraphPin* Input = A->Direction == EGPD_Input ? A : B;
	const auto* TargetNode = Cast<ULxSkillFlowEdGraphNode>(Input->GetOwningNode());
	if (TargetNode && TargetNode->Data && TargetNode->Data->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach
		&& Output->PinName != TEXT("命中"))
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FText::FromString(TEXT("元素异常依附需要前置命中结果，请连接命中出口")));
	TSet<const UEdGraphNode*> Visited;
	TArray<const UEdGraphNode*> Pending {Input->GetOwningNode()};
	while (!Pending.IsEmpty())
	{
		const UEdGraphNode* Node = Pending.Pop();
		if (Node == Output->GetOwningNode()) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FText::FromString(TEXT("最小版本不支持循环")));
		if (Visited.Contains(Node)) continue;
		Visited.Add(Node);
		for (const UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == EGPD_Output)
			for (const UEdGraphPin* Linked : Pin->LinkedTo) Pending.Add(Linked->GetOwningNode());
	}
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::FromString(TEXT("连接技能事件")));
}

bool ULxSkillFlowEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const bool bResult = Super::TryCreateConnection(A, B);
	if (bResult) { auto* Graph = CastChecked<ULxSkillFlowEdGraph>(A->GetOwningNode()->GetGraph()); Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
	return bResult;
}

void ULxSkillFlowEdGraphSchema::BreakNodeLinks(UEdGraphNode& Node) const
{
	Super::BreakNodeLinks(Node);
	if (auto* Graph = Cast<ULxSkillFlowEdGraph>(Node.GetGraph())) { Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
}

void ULxSkillFlowEdGraphSchema::BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const
{
	Super::BreakPinLinks(Pin, bNotify);
	if (auto* Graph = Cast<ULxSkillFlowEdGraph>(Pin.GetOwningNode()->GetGraph())) { Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
}

void ULxSkillFlowEdGraphSchema::BreakSinglePinLink(UEdGraphPin* A, UEdGraphPin* B) const
{
	Super::BreakSinglePinLink(A, B);
	if (A) if (auto* Graph = Cast<ULxSkillFlowEdGraph>(A->GetOwningNode()->GetGraph())) { Graph->SynchronizeAsset(); Graph->NotifyGraphChanged(); }
}
