#include "LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"

bool FLxInteractionTreeFeatures::IsEnabled(ELxInteractionActionType Type) const
{
	switch (Type)
	{
	case ELxInteractionActionType::TreasureChest: return bEnableTreasureChest;
	case ELxInteractionActionType::Warehouse: return bEnableWarehouse;
	case ELxInteractionActionType::TradeContainer: return bEnableTradeContainer;
	case ELxInteractionActionType::TriggerMechanism: return bEnableTriggerMechanism;
	case ELxInteractionActionType::ItemTransfer: return bEnableItemTransfer;
	case ELxInteractionActionType::FunctionPage: return bEnableFunctionPage;
	case ELxInteractionActionType::Quest: return bEnableQuest;
	default: return false;
	}
}

ULxInteractionTreeNodeData* ULxInteractionTreeAsset::FindNode(const FGuid& Id) const
{
	for (ULxInteractionTreeNodeData* Node : Nodes)
	{
		if (Node && Node->NodeId == Id) return Node;
	}
	return nullptr;
}

bool ULxInteractionTreeAsset::IsFunctionType(ELxInteractionActionType Type)
{
	return Type != ELxInteractionActionType::Entrance && Type != ELxInteractionActionType::Dialogue
		&& Type != ELxInteractionActionType::InteractionExit;
}

FText ULxInteractionTreeAsset::GetTypeLabel(ELxInteractionActionType Type)
{
	switch (Type)
	{
	case ELxInteractionActionType::Entrance: return NSLOCTEXT("交互树", "入口", "入口");
	case ELxInteractionActionType::Dialogue: return NSLOCTEXT("交互树", "对话", "对话");
	case ELxInteractionActionType::InteractionExit: return NSLOCTEXT("交互树", "退出", "退出");
	case ELxInteractionActionType::TreasureChest: return NSLOCTEXT("交互树", "TreasureChest", "宝箱");
	case ELxInteractionActionType::Warehouse: return NSLOCTEXT("交互树", "Warehouse", "仓库");
	case ELxInteractionActionType::TradeContainer: return NSLOCTEXT("交互树", "TradeContainer", "交易");
	case ELxInteractionActionType::TriggerMechanism: return NSLOCTEXT("交互树", "TriggerMechanism", "机关");
	case ELxInteractionActionType::ItemTransfer: return NSLOCTEXT("交互树", "ItemTransfer", "物品传递");
	case ELxInteractionActionType::FunctionPage: return NSLOCTEXT("交互树", "FunctionPage", "功能界面");
	case ELxInteractionActionType::Quest: return NSLOCTEXT("交互树", "Quest", "任务");
	default: return FText::GetEmpty();
	}
}

bool ULxInteractionTreeAsset::CanAddNode(ELxInteractionActionType Type) const
{
	if (!StaticEnum<ELxInteractionActionType>()->IsValidEnumValue(static_cast<int64>(Type))) return false;
	if (!IsFunctionType(Type)) return true;
	if (!Features.IsEnabled(Type)) return false;
	if (Features.MultipleNodeTypes.Contains(Type)) return true;
	return !Nodes.ContainsByPredicate([Type](const ULxInteractionTreeNodeData* Node)
	{
		return Node && Node->Type == Type;
	});
}

bool ULxInteractionTreeAsset::ValidateTree(FText& OutError) const
{
	OutError = FText();
	TMap<FGuid, const ULxInteractionTreeNodeData*> Lookup;
	TMap<ELxInteractionActionType, int32> Counts;
	for (const ULxInteractionTreeNodeData* Node : Nodes)
	{
		if (!Node || !Node->NodeId.IsValid() || Lookup.Contains(Node->NodeId))
		{
			OutError = FText::FromString(TEXT("存在空节点或重复节点标识。")); return false;
		}
		Lookup.Add(Node->NodeId, Node);
		if (!StaticEnum<ELxInteractionActionType>()->IsValidEnumValue(static_cast<int64>(Node->Type)))
		{
			OutError = FText::FromString(TEXT("存在无效节点类型。")); return false;
		}
		if (IsFunctionType(Node->Type))
		{
			if (!Features.IsEnabled(Node->Type))
			{
				OutError = FText::Format(NSLOCTEXT("交互树", "功能未启用", "{0}节点对应的功能尚未启用，请启用功能或删除节点。"), GetTypeLabel(Node->Type)); return false;
			}
			if (++Counts.FindOrAdd(Node->Type) > 1 && !Features.MultipleNodeTypes.Contains(Node->Type))
			{
				OutError = FText::Format(NSLOCTEXT("交互树", "重复功能", "{0}默认只能放置一个节点。"), GetTypeLabel(Node->Type)); return false;
			}
		}
		if (Node->Type == ELxInteractionActionType::InteractionExit && !Node->Children.IsEmpty())
		{
			OutError = FText::FromString(TEXT("退出节点不能连接子项。")); return false;
		}
	}
	// 每个节点必须且只能有一个父项；开始节点也计入父项。
	TMap<FGuid, int32> Parents;
	for (const FGuid& Id : Roots)
	{
		if (!Lookup.Contains(Id) || ++Parents.FindOrAdd(Id) > 1)
		{
			OutError = FText::FromString(TEXT("开始节点连接无效或重复。")); return false;
		}
	}
	for (const ULxInteractionTreeNodeData* Node : Nodes)
	{
		for (const FGuid& Child : Node->Children)
		{
			const ULxInteractionTreeNodeData* const* Target = Lookup.Find(Child);
			if (!Target || ++Parents.FindOrAdd(Child) > 1 || (*Target)->Type == ELxInteractionActionType::Entrance)
			{
				OutError = FText::FromString(TEXT("子项连接无效：节点只能有一个父项，入口只能连接开始节点。")); return false;
			}
		}
	}
	TSet<FGuid> Visited;
	TFunction<bool(const FGuid&)> Visit = [&](const FGuid& Id)
	{
		if (Visited.Contains(Id)) return false;
		Visited.Add(Id);
		for (const FGuid& Child : Lookup[Id]->Children)
		{
			if (!Visit(Child)) return false;
		}
		return true;
	};
	for (const FGuid& Id : Roots)
	{
		if (!Visit(Id))
		{
			OutError = FText::FromString(TEXT("交互树存在循环连接。")); return false;
		}
	}
	if (Visited.Num() != Nodes.Num())
	{
		OutError = FText::FromString(TEXT("存在未连接到开始节点的节点或循环，请完成连线。")); return false;
	}
	return true;
}

bool ULxInteractionTreeAsset::CreateRuntimeTree(UObject* Outer, TArray<ULxInteractionNode*>& OutRoots, FText& OutError) const
{
	OutRoots.Reset();
	if (!Outer || !ValidateTree(OutError)) return false;
	TMap<FGuid, ULxInteractionNode*> Instances;
	for (const ULxInteractionTreeNodeData* Data : Nodes)
	{
		ULxInteractionNode* Node = NewObject<ULxInteractionNode>(Outer);
		Node->InitializeInteractionNode(Data->PromptText, Data->Type, {}, Data->Requirement, Data->NpcDialogueText);
		Node->SetQuestInteractionConfig(Data->QuestConfig);
		Node->SetCloseInteractionDialogue(Data->bCloseInteractionDialogue);
		Instances.Add(Data->NodeId, Node);
	}
	for (const ULxInteractionTreeNodeData* Data : Nodes)
	{
		for (const FGuid& Child : Data->Children) Instances[Data->NodeId]->AddChildNode(Instances[Child]);
	}
	for (const FGuid& Id : Roots) OutRoots.Add(Instances[Id]);
	return true;
}
