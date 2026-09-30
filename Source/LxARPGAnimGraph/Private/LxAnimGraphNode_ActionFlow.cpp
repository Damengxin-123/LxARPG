#include "LxAnimGraphNode_ActionFlow.h"

#include "LxAnimGraphNode_Action.h"
#include "Animation/Skeleton.h"
#include "K2Node_Knot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"

namespace
{
/** 跳过重路由节点，返回真正连接的动作源。 */
UEdGraphNode* FindActionInput(UEdGraphPin* Pin)
{
	for (int32 Depth = 0; Pin && Pin->LinkedTo.Num() == 1 && Depth < 64; ++Depth)
	{
		UEdGraphNode* Node = Pin->LinkedTo[0]->GetOwningNode();
		if (UK2Node_Knot* Knot = Cast<UK2Node_Knot>(Node)) Pin = Knot->GetInputPin();
		else return Node;
	}
	return nullptr;
}
}

ULxAnimGraphNode_ActionCache::ULxAnimGraphNode_ActionCache()
{
	Node.Actions.SetNum(2);
}

FText ULxAnimGraphNode_ActionCache::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("动画动作缓存"));
}

FText ULxAnimGraphNode_ActionCache::GetTooltipText() const
{
	return FText::FromString(TEXT("连接多个动作姿势。专用技能优先，其余按输入顺序选择；无匹配时保持当前播放器。右键添加动作输入，或在候选动作数组增删输入。"));
}

void ULxAnimGraphNode_ActionCache::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	ReconstructNode();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void ULxAnimGraphNode_ActionCache::AddActionPin()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("添加动作输入")));
	Modify();
	Node.Actions.AddDefaulted();
	ReconstructNode();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void ULxAnimGraphNode_ActionCache::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	if (!Context->bIsDebugging && !Context->Pin)
	{
		Menu->AddSection(TEXT("动作缓存"), FText::FromString(TEXT("动作缓存"))).AddMenuEntry(
			TEXT("添加动作输入"), FText::FromString(TEXT("添加动作输入")), FText::FromString(TEXT("增加一个候选动作姿势输入")), FSlateIcon(),
			FUIAction(FExecuteAction::CreateUObject(const_cast<ULxAnimGraphNode_ActionCache*>(this), &ULxAnimGraphNode_ActionCache::AddActionPin)));
	}
}

void ULxAnimGraphNode_ActionCache::ValidateAnimNodeDuringCompilation(USkeleton* Skeleton, FCompilerResultsLog& Log)
{
	Super::ValidateAnimNodeDuringCompilation(Skeleton, Log);
	if (Node.Actions.IsEmpty()) Log.Error(TEXT("@@ 至少需要一个候选动作输入。"), this);
	for (int32 Index = 0; Index < Node.Actions.Num(); ++Index)
	{
		if (!Cast<ULxAnimGraphNode_Action>(FindActionInput(FindPin(*FString::Printf(TEXT("Actions_%d"), Index)))))
			Log.Error(TEXT("@@ 候选输入必须连接基础、近战、远程或防御动作节点；可经过重路由。"), this);
	}
}

FText ULxAnimGraphNode_ActionLayer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("动作骨骼分层混合"));
}

FText ULxAnimGraphNode_ActionLayer::GetTooltipText() const
{
	return FText::FromString(TEXT("基础和技能输入分别连接动作缓存。技能匹配时按是否使用骨骼混合选择分层或全身覆盖，技能结束自动恢复基础动作。"));
}

void ULxAnimGraphNode_ActionLayer::ValidateAnimNodeDuringCompilation(USkeleton* Skeleton, FCompilerResultsLog& Log)
{
	Super::ValidateAnimNodeDuringCompilation(Skeleton, Log);
	if (!Cast<ULxAnimGraphNode_ActionCache>(FindActionInput(FindPin(TEXT("BasePose")))) ||
		!Cast<ULxAnimGraphNode_ActionCache>(FindActionInput(FindPin(TEXT("ActionPose")))))
		Log.Error(TEXT("@@ 基础动作和技能动作必须分别连接动画动作缓存。"), this);
	if (Node.BoneFilter.BranchFilters.IsEmpty())
		Log.Warning(TEXT("@@ 尚未配置骨骼分层设置，开启骨骼混合的动作不会覆盖任何骨骼。"), this);
	for (const FBranchFilter& Filter : Node.BoneFilter.BranchFilters)
		if (!Skeleton || Skeleton->GetReferenceSkeleton().FindBoneIndex(Filter.BoneName) == INDEX_NONE)
			Log.Error(TEXT("@@ 骨骼分层设置包含当前骨架不存在的骨骼。"), this);
}

FText ULxAnimGraphNode_ActionStack::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("动作姿势混合堆栈"));
}

FText ULxAnimGraphNode_ActionStack::GetTooltipText() const
{
	return FText::FromString(TEXT("接在动作骨骼分层混合或动作缓存之后，自动平滑动作切换。保存旧姿势、曲线和属性，不重放旧通知。原生 Blend Stack 接收动画资源，不能代替此姿势输入节点。"));
}

void ULxAnimGraphNode_ActionStack::ValidateAnimNodeDuringCompilation(USkeleton* Skeleton, FCompilerResultsLog& Log)
{
	Super::ValidateAnimNodeDuringCompilation(Skeleton, Log);
	UEdGraphNode* Source = FindActionInput(FindPin(TEXT("Source")));
	if (!Cast<ULxAnimGraphNode_ActionLayer>(Source) && !Cast<ULxAnimGraphNode_ActionCache>(Source))
		Log.Error(TEXT("@@ 输入必须连接动作骨骼分层混合或动画动作缓存，以自动识别动作切换。"), this);
	if (!FMath::IsFinite(Node.BlendTime) || Node.BlendTime < 0.0f || Node.MaxBlendDepth < 1 || Node.MaxBlendDepth > 8)
		Log.Error(TEXT("@@ 过渡时间必须为有限非负数，最大过渡层数必须在 1 到 8 之间。"), this);
}
