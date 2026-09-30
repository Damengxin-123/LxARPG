#include "LxAnimNode_ActionFlow.h"

#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "UObject/UnrealType.h"

FLxAnimNode_ActionSource* FLxAnimNode_ActionSource::ResolveSource(FPoseLink& Link, const FAnimationBaseContext& Context)
{
	FAnimNode_Base* Linked = Link.GetLinkNode();
	const IAnimClassInterface* Class = Context.AnimInstanceProxy->GetAnimClassInterface();
	if (!Linked || !Class) return nullptr;
	for (const FStructProperty* Property : Class->GetAnimNodeProperties())
	{
		if (Property->Struct->IsChildOf(StaticStruct()) &&
			Property->ContainerPtrToValuePtr<void>(Context.AnimInstanceProxy->GetAnimInstanceObject()) == Linked)
			return static_cast<FLxAnimNode_ActionSource*>(Linked);
	}
	return nullptr;
}

void FLxAnimNode_ActionCache::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	ActiveIndex = INDEX_NONE;
	Sources.Reset(Actions.Num());
	for (FPoseLink& Action : Actions)
	{
		Action.Initialize(Context);
		Sources.Add(ResolveSource(Action, Context));
	}
}

void FLxAnimNode_ActionCache::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	for (FPoseLink& Action : Actions) Action.CacheBones(Context);
}

int32 FLxAnimNode_ActionCache::SelectAction() const
{
	int32 Selected = INDEX_NONE;
	int32 Priority = MIN_int32;
	for (int32 Index = 0; Index < Sources.Num(); ++Index)
	{
		if (!Sources[Index]) continue;
		const FLxActionPoseState State = Sources[Index]->GetActionState();
		if (State.bActive && State.Priority > Priority)
		{
			Selected = Index;
			Priority = State.Priority;
		}
	}
	return Selected != INDEX_NONE ? Selected : bHoldLastAction ? ActiveIndex : INDEX_NONE;
}

FLxActionPoseState FLxAnimNode_ActionCache::GetActionState() const
{
	const int32 Selected = SelectAction();
	return Sources.IsValidIndex(Selected) && Sources[Selected] ? Sources[Selected]->GetActionState() : FLxActionPoseState();
}

void FLxAnimNode_ActionCache::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	const int32 Selected = SelectAction();
	const bool bRestart = Selected != ActiveIndex;
	ActiveIndex = Selected;
	if (Actions.IsValidIndex(ActiveIndex) && Sources[ActiveIndex])
	{
		Sources[ActiveIndex]->SetCachePlayback(bHoldLastAction, bRestart);
		Actions[ActiveIndex].Update(Context);
	}
}

void FLxAnimNode_ActionCache::Evaluate_AnyThread(FPoseContext& Output)
{
	if (Actions.IsValidIndex(ActiveIndex)) Actions[ActiveIndex].Evaluate(Output);
	else Output.ResetToRefPose();
}

void FLxAnimNode_ActionCache::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(FString::Printf(TEXT("动作缓存：%d"), ActiveIndex));
	if (Actions.IsValidIndex(ActiveIndex)) Actions[ActiveIndex].GatherDebugData(DebugData);
}

void FLxAnimNode_ActionLayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	bFullBody = false;
	Layer.BasePose = BasePose;
	Layer.BlendPoses = { ActionPose };
	Layer.BlendWeights = { 0.0f };
	Layer.LayerSetup = { BoneFilter };
	Layer.bMeshSpaceRotationBlend = bMeshSpaceRotationBlend;
	Layer.Initialize_AnyThread(Context);
	BaseSource = ResolveSource(Layer.BasePose, Context);
	ActionSource = ResolveSource(Layer.BlendPoses[0], Context);
}

void FLxAnimNode_ActionLayer::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	Layer.CacheBones_AnyThread(Context);
}

FLxActionPoseState FLxAnimNode_ActionLayer::GetActionState() const
{
	const FLxActionPoseState BaseState = BaseSource ? BaseSource->GetActionState() : FLxActionPoseState();
	const FLxActionPoseState ActionState = ActionSource ? ActionSource->GetActionState() : FLxActionPoseState();
	if (!ActionState.bActive) return BaseState;
	FLxActionPoseState Result = ActionState;
	Result.Key = ActionState.bUseBoneBlend ? HashCombine(BaseState.Key, ActionState.Key) : ActionState.Key;
	return Result;
}

void FLxAnimNode_ActionLayer::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	const FLxActionPoseState State = ActionSource ? ActionSource->GetActionState() : FLxActionPoseState();
	bFullBody = State.bActive && !State.bUseBoneBlend;
	if (bFullBody) Layer.BlendPoses[0].Update(Context);
	else
	{
		Layer.BlendWeights[0] = State.bActive ? 1.0f : 0.0f;
		Layer.Update_AnyThread(Context);
	}
}

void FLxAnimNode_ActionLayer::Evaluate_AnyThread(FPoseContext& Output)
{
	if (bFullBody) Layer.BlendPoses[0].Evaluate(Output);
	else Layer.Evaluate_AnyThread(Output);
}

void FLxAnimNode_ActionLayer::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(bFullBody ? TEXT("动作混合：全身覆盖") : TEXT("动作混合：骨骼分层"));
	Layer.GatherDebugData(DebugData);
}

void FLxActionPoseSnapshot::Store(const FPoseContext& Pose)
{
	Bones.Reset(Pose.Pose.GetNumBones());
	for (FCompactPoseBoneIndex Index : Pose.Pose.ForEachBoneIndex()) Bones.Add(Pose.Pose[Index]);
	Curve.CopyFrom(Pose.Curve);
	Attributes.CopyFrom(Pose.CustomAttributes);
	Age = 0.0f;
}

void FLxActionPoseSnapshot::Restore(FPoseContext& Pose) const
{
	Pose.Pose.CopyBonesFrom(Bones);
	Pose.Curve.CopyFrom(Curve);
	Pose.CustomAttributes.CopyFrom(Attributes);
}

void FLxAnimNode_ActionStack::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	Source.Initialize(Context);
	ActionSource = FLxAnimNode_ActionSource::ResolveSource(Source, Context);
	Stack.Reset();
	LastInput = {};
	LastOutput = {};
	bHasKey = false;
	bPendingTransition = false;
}

void FLxAnimNode_ActionStack::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	Source.CacheBones(Context);
	Stack.Reset();
	LastInput = {};
	LastOutput = {};
	bPendingTransition = false;
}

void FLxAnimNode_ActionStack::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	Source.Update(Context);
	const uint32 Key = ActionSource ? ActionSource->GetActionState().Key : 0;
	bPendingTransition |= bHasKey && Key != LastKey;
	LastKey = Key;
	bHasKey = true;
	for (FLxActionPoseSnapshot& Pose : Stack) Pose.Age += FMath::Max(0.0f, Context.GetDeltaTime());
	// 新一层已经完成时，其之前的历史对最终结果也没有贡献。
	for (int32 Index = Stack.Num() - 1; Index >= 0; --Index)
	{
		if (Stack[Index].Age >= BlendTime)
		{
			Stack.RemoveAt(0, Index + 1);
			break;
		}
	}
}

void FLxAnimNode_ActionStack::Evaluate_AnyThread(FPoseContext& Output)
{
	Source.Evaluate(Output);
	const bool bCompatible = LastInput.Bones.Num() == Output.Pose.GetNumBones() && !LastInput.Bones.IsEmpty();
	if (bPendingTransition && bCompatible && FMath::IsFinite(BlendTime) && BlendTime > SMALL_NUMBER)
	{
		if (Stack.Num() >= FMath::Clamp(MaxBlendDepth, 1, 8))
		{
			Stack.Reset();
			Stack.Add(LastOutput);
		}
		else Stack.Add(LastInput);
		Stack.Last().Age = 0.0f;
	}
	bPendingTransition = false;
	if (!bCompatible || !FMath::IsFinite(BlendTime) || BlendTime <= SMALL_NUMBER) Stack.Reset();
	LastInput.Store(Output);
	if (!Stack.IsEmpty())
	{
		FPoseContext Accumulated(Output), Target(Output), Blended(Output);
		Stack[0].Restore(Accumulated);
		for (int32 Index = 0; Index < Stack.Num(); ++Index)
		{
			if (Index + 1 < Stack.Num()) Stack[Index + 1].Restore(Target);
			else LastInput.Restore(Target);
			const float Linear = FMath::Clamp(Stack[Index].Age / BlendTime, 0.0f, 1.0f);
			const float Alpha = Linear * Linear * (3.0f - 2.0f * Linear);
			FAnimationPoseData ResultData(Blended);
			FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(Accumulated), FAnimationPoseData(Target), 1.0f - Alpha, ResultData);
			Accumulated = Blended;
		}
		Output = Accumulated;
	}
	LastOutput.Store(Output);
}

void FLxAnimNode_ActionStack::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(FString::Printf(TEXT("动作姿势混合堆栈：%d 层"), Stack.Num()));
	Source.GatherDebugData(DebugData);
}
