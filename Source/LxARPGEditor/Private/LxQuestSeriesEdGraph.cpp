#include "LxQuestSeriesEdGraph.h"

#include "EdGraph/EdGraphPin.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestSeriesAsset.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "LxQuestSeriesEdGraph"

const FName ULxQuestSeriesEdGraphSchema::PC_QuestFlow(TEXT("QuestFlow"));

ULxQuestSeriesAsset* ULxQuestSeriesEdGraph::GetQuestSeriesAsset() const
{
	return Cast<ULxQuestSeriesAsset>(GetOuter());
}

void ULxQuestSeriesEdGraph::SynchronizeLinksToAsset() const
{
	ULxQuestSeriesAsset* QuestSeriesAsset = GetQuestSeriesAsset();
	if (!QuestSeriesAsset)
	{
		return;
	}

	TArray<FLxQuestNodeLink> NewLinks;
	for (const UEdGraphNode* GraphNode : Nodes)
	{
		const ULxQuestSeriesEdGraphNode* QuestGraphNode = Cast<ULxQuestSeriesEdGraphNode>(GraphNode);
		if (!QuestGraphNode)
		{
			continue;
		}

		const FLxQuestNodeDefinition* SourceQuestNode = QuestSeriesAsset->FindQuestNodeByEditorId(
			QuestGraphNode->GetQuestEditorNodeId());
		if (!SourceQuestNode || !SourceQuestNode->QuestId.IsValid())
		{
			continue;
		}

		for (const UEdGraphPin* Pin : QuestGraphNode->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				const ULxQuestSeriesEdGraphNode* TargetNode = LinkedPin
					? Cast<ULxQuestSeriesEdGraphNode>(LinkedPin->GetOwningNode())
					: nullptr;
				const FLxQuestNodeDefinition* TargetQuestNode = TargetNode
					? QuestSeriesAsset->FindQuestNodeByEditorId(TargetNode->GetQuestEditorNodeId())
					: nullptr;
				if (!TargetNode || TargetNode == QuestGraphNode || !TargetQuestNode || !TargetQuestNode->QuestId.IsValid())
				{
					continue;
				}

				FLxQuestNodeLink QuestLink;
				QuestLink.FromQuestId = SourceQuestNode->QuestId;
				QuestLink.ToQuestId = TargetQuestNode->QuestId;
				NewLinks.AddUnique(QuestLink);
			}
		}
	}

	if (QuestSeriesAsset->GetQuestLinks() != NewLinks)
	{
		QuestSeriesAsset->Modify();
		QuestSeriesAsset->EditQuestLinks() = MoveTemp(NewLinks);
		QuestSeriesAsset->MarkPackageDirty();
	}
}

void ULxQuestSeriesEdGraphNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, ULxQuestSeriesEdGraphSchema::PC_QuestFlow, TEXT("前置"));
	CreatePin(EGPD_Output, ULxQuestSeriesEdGraphSchema::PC_QuestFlow, TEXT("后续"));
}

void ULxQuestSeriesEdGraphNode::PostLoad()
{
	Super::PostLoad();
	if (!QuestEditorNodeId.IsValid() && QuestNodeId_DEPRECATED.IsValid())
	{
		QuestEditorNodeId = QuestNodeId_DEPRECATED;
	}
}

FText ULxQuestSeriesEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const ULxQuestSeriesEdGraph* QuestGraph = Cast<ULxQuestSeriesEdGraph>(GetGraph());
	const ULxQuestSeriesAsset* QuestSeriesAsset = QuestGraph ? QuestGraph->GetQuestSeriesAsset() : nullptr;
	const FLxQuestNodeDefinition* QuestNode = QuestSeriesAsset
		? QuestSeriesAsset->FindQuestNodeByEditorId(QuestEditorNodeId)
		: nullptr;
	if (!QuestNode)
	{
		return LOCTEXT("MissingQuestNode", "无效任务节点");
	}

	if (!QuestNode->DisplayName.IsEmpty())
	{
		return QuestNode->DisplayName;
	}

	if (QuestNode->QuestId.IsValid())
	{
		return FText::FromName(QuestNode->QuestId.GetTagName());
	}

	return QuestNode->DeveloperName.IsNone()
		? LOCTEXT("UnassignedQuestId", "未选择任务ID")
		: FText::FromName(QuestNode->DeveloperName);
}

FText ULxQuestSeriesEdGraphNode::GetTooltipText() const
{
	const ULxQuestSeriesEdGraph* QuestGraph = Cast<ULxQuestSeriesEdGraph>(GetGraph());
	const ULxQuestSeriesAsset* QuestSeriesAsset = QuestGraph ? QuestGraph->GetQuestSeriesAsset() : nullptr;
	const FLxQuestNodeDefinition* QuestNode = QuestSeriesAsset
		? QuestSeriesAsset->FindQuestNodeByEditorId(QuestEditorNodeId)
		: nullptr;
	return QuestNode && !QuestNode->DisplayDescription.IsEmpty()
		? QuestNode->DisplayDescription
		: GetNodeTitle(ENodeTitleType::FullTitle);
}

FLinearColor ULxQuestSeriesEdGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(0.08f, 0.32f, 0.55f, 1.0f);
}

void ULxQuestSeriesEdGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin || !GetSchema())
	{
		return;
	}

	UEdGraphPin* TargetPin = nullptr;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction != FromPin->Direction)
		{
			TargetPin = Pin;
			break;
		}
	}

	if (TargetPin)
	{
		GetSchema()->TryCreateConnection(FromPin, TargetPin);
	}
}

FName FLxQuestSeriesSchemaAction_NewNode::GetTypeId() const
{
	static const FName TypeId(TEXT("FLxQuestSeriesSchemaAction_NewNode"));
	return TypeId;
}

UEdGraphNode* FLxQuestSeriesSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2f& Location, bool bSelectNewNode)
{
	if (!ParentGraph || !QuestEditorNodeId.IsValid())
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddQuestGraphNode", "添加任务关系节点"));
	ParentGraph->Modify();

	FGraphNodeCreator<ULxQuestSeriesEdGraphNode> NodeCreator(*ParentGraph);
	ULxQuestSeriesEdGraphNode* NewNode = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
	NewNode->SetQuestEditorNodeId(QuestEditorNodeId);
	NewNode->NodePosX = FMath::RoundToInt(Location.X);
	NewNode->NodePosY = FMath::RoundToInt(Location.Y);
	NodeCreator.Finalize();
	NewNode->AutowireNewNode(FromPin);
	ParentGraph->NotifyGraphChanged();
	ParentGraph->MarkPackageDirty();
	return NewNode;
}

void ULxQuestSeriesEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const ULxQuestSeriesEdGraph* QuestGraph = Cast<ULxQuestSeriesEdGraph>(ContextMenuBuilder.CurrentGraph);
	const ULxQuestSeriesAsset* QuestSeriesAsset = QuestGraph ? QuestGraph->GetQuestSeriesAsset() : nullptr;
	if (!QuestSeriesAsset)
	{
		return;
	}

	TSet<FGuid> PlacedNodeIds;
	for (const UEdGraphNode* GraphNode : QuestGraph->Nodes)
	{
		if (const ULxQuestSeriesEdGraphNode* QuestGraphNode = Cast<ULxQuestSeriesEdGraphNode>(GraphNode))
		{
			PlacedNodeIds.Add(QuestGraphNode->GetQuestEditorNodeId());
		}
	}

	for (const FLxQuestNodeDefinition& QuestNode : QuestSeriesAsset->GetQuestNodes())
	{
		if (!QuestNode.EditorNodeId.IsValid() || PlacedNodeIds.Contains(QuestNode.EditorNodeId))
		{
			continue;
		}

		const FText MenuDescription = !QuestNode.DisplayName.IsEmpty()
			? QuestNode.DisplayName
			: FText::FromName(QuestNode.DeveloperName);
		TSharedRef<FLxQuestSeriesSchemaAction_NewNode> Action = MakeShared<FLxQuestSeriesSchemaAction_NewNode>(
			LOCTEXT("QuestNodeCategory", "当前系列任务节点"),
			MenuDescription,
			QuestNode.DisplayDescription,
			0);
		Action->QuestEditorNodeId = QuestNode.EditorNodeId;
		ContextMenuBuilder.AddAction(Action);
	}
}

const FPinConnectionResponse ULxQuestSeriesEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InvalidPin", "任务引脚无效"));
	}

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNode", "任务节点不能连接自身"));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "必须从后续输出连接到前置输入"));
	}

	if (A->GetOwningNode()->GetGraph() != B->GetOwningNode()->GetGraph())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("DifferentGraph", "只能连接当前任务系列中的节点"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("ConnectQuestNodes", "创建任务依赖关系"));
}

bool ULxQuestSeriesEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const bool bConnected = Super::TryCreateConnection(A, B);
	if (bConnected && A && A->GetOwningNode() && A->GetOwningNode()->GetGraph())
	{
		A->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
	return bConnected;
}

void ULxQuestSeriesEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);
	if (TargetNode.GetGraph())
	{
		TargetNode.GetGraph()->NotifyGraphChanged();
	}
}

void ULxQuestSeriesEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
	if (TargetPin.GetOwningNode() && TargetPin.GetOwningNode()->GetGraph())
	{
		TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

void ULxQuestSeriesEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	Super::BreakSinglePinLink(SourcePin, TargetPin);
	if (SourcePin && SourcePin->GetOwningNode() && SourcePin->GetOwningNode()->GetGraph())
	{
		SourcePin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

FLinearColor ULxQuestSeriesEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor(0.12f, 0.55f, 0.95f, 1.0f);
}

#undef LOCTEXT_NAMESPACE
