#include "LxAnimalAssetLibrary.h"

#include "LxAIBehaviorTreeEdGraph.h"
#include "Animation/AnimBlueprint.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/Character.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"

FString ULxAnimalAssetLibrary::ConfigureAnimalBehavior(ULxAIBehaviorTreeAsset* Asset)
{
    if (!Asset) return TEXT("行为资产为空");
    Asset->Modify();
    Asset->Nodes.Reset();
    Asset->Roots.Reset();
    // 只对脚本新建的独立行为资产重建图，原参考资产不参与修改。
    ULxAIBehaviorTreeEdGraph* Graph = NewObject<ULxAIBehaviorTreeEdGraph>(Asset, NAME_None, RF_Transactional);
    Graph->Schema = ULxAIBehaviorTreeEdGraphSchema::StaticClass();
    Asset->EditorGraph = Graph;
    Graph->EnsureEntryNodes();

    // 创建中文命名的可视节点，稳定标识和布局由同一处统一生成。
    auto AddNode = [Graph, Asset](ELxAIBehaviorNodeKind Kind, ELxAIBehaviorState State,
        ELxAIBehaviorAction Action, const TCHAR* Label, int32 X, int32 Y)
    {
        ULxAIBehaviorTreeNodeData* Data = NewObject<ULxAIBehaviorTreeNodeData>(Asset, NAME_None, RF_Transactional);
        Data->Kind = Kind;
        Data->State = State;
        Data->Action = Action;
        Data->MotionType = Data->GetMotionType();
        Data->Label = FText::FromString(Label);
        Data->NodeId = FGuid::NewGuid();
        if (State != ELxAIBehaviorState::Idle) Data->HealthRange.Min = UE_SMALL_NUMBER;
        FGraphNodeCreator<ULxAIBehaviorTreeEdGraphNode> Creator(*Graph);
        ULxAIBehaviorTreeEdGraphNode* Node = Creator.CreateNode();
        Node->Data = Data;
        Node->NodePosX = X;
        Node->NodePosY = Y;
        Creator.Finalize();
        return Node;
    };
    // 连接父子流程引脚；最后统一同步序列化数据并校验层级契约。
    auto Link = [](ULxAIBehaviorTreeEdGraphNode* Parent, ULxAIBehaviorTreeEdGraphNode* Child)
    {
        for (UEdGraphPin* Output : Parent->Pins)
            if (Output->Direction == EGPD_Output)
                for (UEdGraphPin* Input : Child->Pins)
                    if (Input->Direction == EGPD_Input) Output->MakeLinkTo(Input);
    };
    ULxAIBehaviorTreeEdGraphNode* Patrol = AddNode(ELxAIBehaviorNodeKind::State, ELxAIBehaviorState::Patrol, ELxAIBehaviorAction::Wait, TEXT("闲逛"), 380, 0);
    ULxAIBehaviorTreeEdGraphNode* PatrolPhase = AddNode(ELxAIBehaviorNodeKind::Phase, ELxAIBehaviorState::Patrol, ELxAIBehaviorAction::Wait, TEXT("范围闲逛"), 760, 0);
    ULxAIBehaviorTreeEdGraphNode* PatrolAction = AddNode(ELxAIBehaviorNodeKind::Action, ELxAIBehaviorState::Patrol, ELxAIBehaviorAction::SpawnPointPatrol, TEXT("走走停停"), 1140, 0);
    PatrolAction->Data->WaitSeconds = 3.0f;
    Link(Patrol, PatrolPhase);
    Link(PatrolPhase, PatrolAction);

    ULxAIBehaviorTreeEdGraphNode* Flee = AddNode(ELxAIBehaviorNodeKind::State, ELxAIBehaviorState::Flee, ELxAIBehaviorAction::Wait, TEXT("逃跑"), 380, 650);
    ULxAIBehaviorTreeEdGraphNode* FleePhase = AddNode(ELxAIBehaviorNodeKind::Phase, ELxAIBehaviorState::Flee, ELxAIBehaviorAction::Wait, TEXT("远离威胁"), 760, 650);
    ULxAIBehaviorTreeEdGraphNode* FleeAction = AddNode(ELxAIBehaviorNodeKind::Action, ELxAIBehaviorState::Flee, ELxAIBehaviorAction::RandomFlee, TEXT("跑向安全处"), 1140, 650);
    FleeAction->Data->FleeStepMeters = 6.0f;
    FleeAction->Data->FleeSafeDistanceMeters = 20.0f;
    Link(Flee, FleePhase);
    Link(FleePhase, FleeAction);

    ULxAIBehaviorTreeEdGraphNode* Death = AddNode(ELxAIBehaviorNodeKind::State, ELxAIBehaviorState::Idle, ELxAIBehaviorAction::Wait, TEXT("死亡"), 380, 1300);
    ULxAIBehaviorTreeEdGraphNode* DeathPhase = AddNode(ELxAIBehaviorNodeKind::Phase, ELxAIBehaviorState::Idle, ELxAIBehaviorAction::Wait, TEXT("进入死亡"), 760, 1300);
    ULxAIBehaviorTreeEdGraphNode* DeathAction = AddNode(ELxAIBehaviorNodeKind::Action, ELxAIBehaviorState::Idle, ELxAIBehaviorAction::EnterDeath, TEXT("进入死亡"), 1140, 1300);
    Death->Data->HealthRange.Max = 0.0f;
    DeathPhase->Data->HealthRange.Max = 0.0f;
    Link(Death, DeathPhase);
    Link(DeathPhase, DeathAction);
    for (UEdGraphNode* Raw : Graph->Nodes)
    {
        ULxAIBehaviorTreeEdGraphNode* Node = Cast<ULxAIBehaviorTreeEdGraphNode>(Raw);
        if (!Node || !Node->Data || Node->Data->Kind != ELxAIBehaviorNodeKind::Entry) continue;
        const ELxAIBehaviorEntry Entry = Node->Data->Entry;
        Node->NodePosY = static_cast<int32>(Entry) * 325;
        Link(Node, Entry == ELxAIBehaviorEntry::Calm ? Patrol : Entry == ELxAIBehaviorEntry::CharacterDeath ? Death : Flee);
    }
    Asset->Analysis.NearEnterDistanceMeters = 5.0f;
    Asset->Analysis.NearExitDistanceMeters = 8.0f;
    Asset->Perception.SightRadiusMeters = 12.0f;
    Asset->Perception.LoseSightRadiusMeters = 18.0f;
    Asset->Perception.HearingRadiusMeters = 10.0f;
    Graph->SynchronizeAsset();
    Asset->MarkPackageDirty();
    return ValidateAnimalBehavior(Asset);
}

FString ULxAnimalAssetLibrary::ValidateAnimalBehavior(ULxAIBehaviorTreeAsset* Asset)
{
    if (!Asset) return TEXT("行为资产为空");
    FText Error;
    if (!Asset->ValidateConfiguration(Error)) return Error.ToString();
    for (const ULxAIBehaviorTreeNodeData* Node : Asset->Nodes)
        if (Node && Node->Kind == ELxAIBehaviorNodeKind::Action &&
            Node->Action != ELxAIBehaviorAction::SpawnPointPatrol &&
            Node->Action != ELxAIBehaviorAction::RandomFlee && Node->Action != ELxAIBehaviorAction::EnterDeath)
            return TEXT("小动物行为树含有闲逛、逃跑及死亡以外的动作");
    return FString();
}

int32 ULxAnimalAssetLibrary::RemoveCombatAnimationNodes(UAnimBlueprint* Blueprint)
{
    if (!Blueprint) return 0;
    Blueprint->Modify();
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    int32 Removed = 0;
    for (UEdGraph* Graph : Graphs)
    {
        // 小动物不需要技能分层；让基础动作缓存直接进入姿势过渡堆栈。
        const TArray<TObjectPtr<UEdGraphNode>> LayerNodes = Graph->Nodes;
        for (UEdGraphNode* Layer : LayerNodes)
        {
            if (!Layer || Layer->GetClass()->GetFName() != TEXT("LxAnimGraphNode_ActionLayer")) continue;
            UEdGraphPin* Base = Layer->FindPin(TEXT("BasePose"));
            UEdGraphPin* Action = Layer->FindPin(TEXT("ActionPose"));
            UEdGraphNode* ActionCache = Action && Action->LinkedTo.Num() == 1 ? Action->LinkedTo[0]->GetOwningNode() : nullptr;
            if (Base && Base->LinkedTo.Num() == 1)
            {
                UEdGraphPin* BaseOutput = Base->LinkedTo[0];
                for (UEdGraphPin* Output : Layer->Pins)
                    if (Output->Direction == EGPD_Output)
                    {
                        const TArray<UEdGraphPin*> Destinations = Output->LinkedTo;
                        for (UEdGraphPin* Destination : Destinations)
                        {
                            Output->BreakLinkTo(Destination);
                            BaseOutput->MakeLinkTo(Destination);
                        }
                    }
            }
            FBlueprintEditorUtils::RemoveNode(Blueprint, Layer, true);
            if (ActionCache && ActionCache->GetClass()->GetFName() == TEXT("LxAnimGraphNode_ActionCache"))
                FBlueprintEditorUtils::RemoveNode(Blueprint, ActionCache, true);
        }
        const TArray<TObjectPtr<UEdGraphNode>> Nodes = Graph->Nodes;
        for (UEdGraphNode* Node : Nodes)
            if (Node && (Node->GetClass()->GetFName() == TEXT("LxAnimGraphNode_MeleeAction") ||
                Node->GetClass()->GetFName() == TEXT("LxAnimGraphNode_RangedAction")))
            {
                FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
                ++Removed;
            }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    return Removed;
}

FString ULxAnimalAssetLibrary::ConfigureFollowerMesh(UBlueprint* Blueprint, FName ComponentName)
{
    UEdGraph* Graph = Blueprint ? FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) : nullptr;
    if (!Graph) return TEXT("缺少角色构造脚本");
    for (UEdGraphNode* Node : Graph->Nodes)
        if (Node && Node->NodeComment == TEXT("同步小羊头顶毛")) return FString();
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    if (!Entry) return TEXT("缺少构造脚本入口");
    UEdGraphPin* Then = Entry->FindPin(UEdGraphSchema_K2::PN_Then);
    if (!Then) return TEXT("缺少构造执行引脚");
    Graph->Modify();
    Blueprint->Modify();
    // 创建读取附属组件和角色原生网格组件的变量节点。
    auto Getter = [Graph](FName Name, UClass* Owner, int32 Y)
    {
        FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);
        UK2Node_VariableGet* Node = Creator.CreateNode();
        if (Owner) Node->VariableReference.SetExternalMember(Name, Owner);
        else Node->VariableReference.SetSelfMember(Name);
        Node->NodePosX = 100;
        Node->NodePosY = Y;
        Creator.Finalize();
        return Node;
    };
    UK2Node_VariableGet* Follower = Getter(ComponentName, nullptr, 180);
    UK2Node_VariableGet* Leader = Getter(TEXT("Mesh"), nullptr, 330);
    FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
    UK2Node_CallFunction* Call = Creator.CreateNode();
    Call->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(USkinnedMeshComponent, SetLeaderPoseComponent), USkinnedMeshComponent::StaticClass());
    Call->NodeComment = TEXT("同步小羊头顶毛");
    Call->NodePosX = 420;
    Creator.Finalize();
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    if (!Schema->TryCreateConnection(Follower->GetValuePin(), Call->FindPinChecked(UEdGraphSchema_K2::PN_Self)) ||
        !Schema->TryCreateConnection(Leader->GetValuePin(), Call->FindPinChecked(TEXT("NewLeaderBoneComponent"))))
        return TEXT("附属网格数据连线失败");
    // 插入入口后并保留已有构造逻辑的后续执行链。
    const TArray<UEdGraphPin*> Continuations = Then->LinkedTo;
    Then->BreakAllPinLinks();
    Schema->TryCreateConnection(Then, Call->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
    for (UEdGraphPin* Next : Continuations)
        Schema->TryCreateConnection(Call->FindPinChecked(UEdGraphSchema_K2::PN_Then), Next);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    return FString();
}
