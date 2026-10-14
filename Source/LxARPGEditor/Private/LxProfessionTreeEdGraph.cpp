#include "LxProfessionTreeEdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

namespace
{
	/** 把连线变更写入资产，并通知所有已打开的视图。 */
	void SynchronizeProfessionGraph(UEdGraph* Graph)
	{
		if (auto* Tree = Cast<ULxProfessionTreeEdGraph>(Graph))
		{ Tree->SynchronizeAsset(); Tree->NotifyGraphChanged(); Tree->MarkPackageDirty(); }
	}
}

ULxProfessionTreeAsset* ULxProfessionTreeEdGraph::GetAsset() const
{
	return Cast<ULxProfessionTreeAsset>(GetOuter());
}

void ULxProfessionTreeEdGraph::SynchronizeAsset()
{
	ULxProfessionTreeAsset* Asset = GetAsset();
	if (!Asset) return;
	TArray<TObjectPtr<ULxProfessionTreeInfluence>> Updated;
	for (UEdGraphNode* Node : Nodes)
	{
		auto* Source = Cast<ULxProfessionTreeEdGraphNode>(Node);
		if (!Source || !Source->Definition || !Asset->Professions.Contains(Source->Definition)) continue;
		UEdGraphPin* Output = Source->FindPin(TEXT("影响"), EGPD_Output);
		if (!Output) continue;
		for (UEdGraphPin* Input : Output->LinkedTo)
		{
			auto* Target = Input ? Cast<ULxProfessionTreeEdGraphNode>(Input->GetOwningNode()) : nullptr;
			if (!Target || !Target->Definition || Target == Source || !Asset->Professions.Contains(Target->Definition)) continue;
			const ELxProfessionTreeRelation Kind = Input->PinName == TEXT("职业前置")
				? ELxProfessionTreeRelation::Prerequisite : ELxProfessionTreeRelation::Benefit;
			ULxProfessionTreeInfluence* Relation = nullptr;
			for (ULxProfessionTreeInfluence* Existing : Asset->Influences)
				if (Existing && Existing->Source == Source->Definition && Existing->Target == Target->Definition && Existing->Kind == Kind)
				{ Relation = Existing; break; }
			if (!Relation)
			{
				Relation = NewObject<ULxProfessionTreeInfluence>(Asset, NAME_None, RF_Transactional);
				Relation->Source = Source->Definition; Relation->Target = Target->Definition; Relation->Kind = Kind;
			}
			Updated.AddUnique(Relation);
		}
	}
	if (Updated != Asset->Influences)
	{ Asset->Modify(); Asset->Influences = MoveTemp(Updated); Asset->MarkPackageDirty(); }
}

void ULxProfessionTreeEdGraphNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, TEXT("ProfessionPrerequisite"), TEXT("职业前置"));
	CreatePin(EGPD_Input, TEXT("ProfessionBenefit"), TEXT("职业增益"));
	CreatePin(EGPD_Output, TEXT("ProfessionOutput"), TEXT("影响"));
}

FText ULxProfessionTreeEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!Definition) return FText::FromString(TEXT("无效职业"));
	return FText::Format(FText::FromString(TEXT("{0}\n{1}")),
		StaticEnum<ELxProfessionTreeCategory>()->GetDisplayNameTextByValue(static_cast<int64>(Definition->ProfessionCategory)),
		Definition->DisplayName.IsEmpty() ? FText::FromString(TEXT("未命名职业")) : Definition->DisplayName);
}

FText ULxProfessionTreeEdGraphNode::GetTooltipText() const
{
	return Definition ? Definition->Description : FText::GetEmpty();
}

FLinearColor ULxProfessionTreeEdGraphNode::GetNodeTitleColor() const
{
	if (!Definition) return FLinearColor::Gray;
	switch (Definition->ProfessionCategory)
	{
	case ELxProfessionTreeCategory::Combat: return FLinearColor(0.55f, 0.12f, 0.09f);
	case ELxProfessionTreeCategory::Gathering: return FLinearColor(0.08f, 0.36f, 0.2f);
	default: return FLinearColor(0.12f, 0.25f, 0.55f);
	}
}

void ULxProfessionTreeEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	// 输出拖线有两种目标含义，不擅自选择；从输入拖线则唯一对应输出。
	if (FromPin && FromPin->Direction == EGPD_Input)
		GetSchema()->TryCreateConnection(FindPin(TEXT("影响"), EGPD_Output), FromPin);
}

UEdGraphNode* FLxProfessionTreePlaceAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2f& Location, bool bSelectNewNode)
{
	auto* Graph = Cast<ULxProfessionTreeEdGraph>(ParentGraph);
	if (!Graph || !Definition.IsValid() || !Graph->GetAsset() || !Graph->GetAsset()->Professions.Contains(Definition.Get())) return nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
		if (auto* Existing = Cast<ULxProfessionTreeEdGraphNode>(Node); Existing && Existing->Definition == Definition.Get()) return Existing;
	const FScopedTransaction Transaction(FText::FromString(TEXT("放置职业节点")));
	Graph->Modify();
	FGraphNodeCreator<ULxProfessionTreeEdGraphNode> Creator(*Graph);
	auto* Node = Creator.CreateUserInvokedNode(bSelectNewNode);
	Node->Definition = Definition.Get();
	Node->NodePosX = FMath::RoundToInt(Location.X); Node->NodePosY = FMath::RoundToInt(Location.Y);
	Creator.Finalize();
	Node->AutowireNewNode(FromPin);
	SynchronizeProfessionGraph(Graph);
	return Node;
}

void ULxProfessionTreeEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& Builder) const
{
	const auto* Graph = Cast<ULxProfessionTreeEdGraph>(Builder.CurrentGraph);
	const auto* Asset = Graph ? Graph->GetAsset() : nullptr;
	if (!Asset) return;
	TSet<ULxProfessionTreeDefinition*> Placed;
	for (UEdGraphNode* Node : Graph->Nodes)
		if (auto* Typed = Cast<ULxProfessionTreeEdGraphNode>(Node)) Placed.Add(Typed->Definition);
	for (ULxProfessionTreeDefinition* Definition : Asset->Professions)
	{
		if (!Definition || Placed.Contains(Definition)) continue;
		auto Action = MakeShared<FLxProfessionTreePlaceAction>(
			StaticEnum<ELxProfessionTreeCategory>()->GetDisplayNameTextByValue(static_cast<int64>(Definition->ProfessionCategory)),
			Definition->DisplayName, Definition->Description, 0);
		Action->Definition = Definition; Builder.AddAction(Action);
	}
}

const FPinConnectionResponse ULxProfessionTreeEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B || A->Direction == B->Direction || A->GetOwningNode() == B->GetOwningNode()
		|| A->GetOwningNode()->GetGraph() != B->GetOwningNode()->GetGraph())
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("请将一个职业的输出连接到另一个职业的需求输入"));
	const UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
	const UEdGraphPin* Input = A->Direction == EGPD_Input ? A : B;
	const auto* Source = Cast<ULxProfessionTreeEdGraphNode>(Output->GetOwningNode());
	const auto* Target = Cast<ULxProfessionTreeEdGraphNode>(Input->GetOwningNode());
	if (!Source || !Target || !Source->Definition || !Target->Definition || Output->PinName != TEXT("影响")
		|| (Input->PinName != TEXT("职业前置") && Input->PinName != TEXT("职业增益")))
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("无效的职业关系引脚"));
	if (Output->LinkedTo.Contains(Input)) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("该关系已经存在"));
	if (Input->PinName == TEXT("职业前置"))
	{
		// 新增来源到目标的前置边之前，从目标寻找来源，拒绝构成前置环。
		TArray<const UEdGraphNode*> Pending { Target };
		TSet<const UEdGraphNode*> Visited;
		while (!Pending.IsEmpty())
		{
			const UEdGraphNode* Current = Pending.Pop();
			if (Current == Source) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("前置关系不能形成循环"));
			if (Visited.Contains(Current)) continue;
			Visited.Add(Current);
			const UEdGraphPin* Pin = Current->FindPin(TEXT("影响"), EGPD_Output);
			if (Pin) for (const UEdGraphPin* Linked : Pin->LinkedTo)
				if (Linked && Linked->PinName == TEXT("职业前置")) Pending.Add(Linked->GetOwningNode());
		}
	}
	else if (const auto* Graph = Cast<ULxProfessionTreeEdGraph>(Source->GetGraph()))
	{
		for (const ULxProfessionTreeInfluence* Relation : Graph->GetAsset()->Influences)
			if (Relation && Relation->Kind == ELxProfessionTreeRelation::Benefit && Relation->bMutual
				&& Relation->Source == Target->Definition && Relation->Target == Source->Definition)
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("已有相互影响，无需再添加反向增益"));
	}
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("创建职业影响"));
}

bool ULxProfessionTreeEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("连接职业影响")));
	const bool Result = Super::TryCreateConnection(A, B);
	if (Result) SynchronizeProfessionGraph(A->GetOwningNode()->GetGraph());
	return Result;
}

void ULxProfessionTreeEdGraphSchema::BreakNodeLinks(UEdGraphNode& Node) const
{
	Super::BreakNodeLinks(Node); SynchronizeProfessionGraph(Node.GetGraph());
}

void ULxProfessionTreeEdGraphSchema::BreakPinLinks(UEdGraphPin& Pin, bool bNotify) const
{
	Super::BreakPinLinks(Pin, bNotify); SynchronizeProfessionGraph(Pin.GetOwningNode()->GetGraph());
}

void ULxProfessionTreeEdGraphSchema::BreakSinglePinLink(UEdGraphPin* Source, UEdGraphPin* Target) const
{
	Super::BreakSinglePinLink(Source, Target);
	if (Source) SynchronizeProfessionGraph(Source->GetOwningNode()->GetGraph());
}

FLinearColor ULxProfessionTreeEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == TEXT("ProfessionPrerequisite")) return FLinearColor(1.f, 0.65f, 0.15f);
	if (PinType.PinCategory == TEXT("ProfessionBenefit")) return FLinearColor(0.15f, 0.8f, 0.5f);
	return FLinearColor(0.65f, 0.75f, 0.95f);
}
