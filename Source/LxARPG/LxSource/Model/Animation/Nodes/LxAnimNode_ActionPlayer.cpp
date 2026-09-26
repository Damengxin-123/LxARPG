#include "LxAnimNode_ActionPlayer.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"

void FLxAnimNode_ActionPlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	bWasMatched = false;
	bHasPlayed = false;
	LastRequestId = INDEX_NONE;
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		MontageSlot.SlotName = Montage->SlotAnimTracks.IsEmpty() ? NAME_None : Montage->SlotAnimTracks[0].SlotName;
		MontageSlot.Initialize_AnyThread(Context);
	}
	else if (UBlendSpace* Blend = Cast<UBlendSpace>(Animation))
	{
		BlendPlayer.SetBlendSpace(Blend);
		BlendPlayer.Initialize_AnyThread(Context);
	}
	else
	{
		SequencePlayer.SetSequence(Cast<UAnimSequenceBase>(Animation));
		SequencePlayer.Initialize_AnyThread(Context);
	}
}

void FLxAnimNode_ActionPlayer::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	if (Cast<UAnimMontage>(Animation)) MontageSlot.CacheBones_AnyThread(Context);
	else if (Cast<UBlendSpace>(Animation)) BlendPlayer.CacheBones_AnyThread(Context);
	else SequencePlayer.CacheBones_AnyThread(Context);
}

void FLxAnimNode_ActionPlayer::PreUpdate(const UAnimInstance* InAnimInstance)
{
	const ULxAnimInstanceBase* Instance = Cast<ULxAnimInstanceBase>(InAnimInstance);
	bMatched = Instance && Animation && Instance->MatchesMotion(bAttackChannel, MotionType, SkillId);
	if (Instance)
	{
		AutomaticPlayRate = bAttackChannel ? Instance->CurrentActionAnimationPlayRate : Instance->CurrentBaseAnimationPlayRate;
		if (!FMath::IsFinite(AutomaticPlayRate)) AutomaticPlayRate = 1.0f;
		bLoop = bAttackChannel ? (MotionType == ELxCharacterMotionType::Defend || Instance->bCurrentActionAnimationLoop) : Instance->bCurrentBaseAnimationLoop;
		RequestId = bAttackChannel ? Instance->ActionAnimationPlayRequestId : 0;
	}
	if (UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		// 原生蒙太奇操作仅允许在游戏线程执行；图首次相关后一帧开始播放。
		UAnimInstance* MutableInstance = const_cast<UAnimInstance*>(InAnimInstance);
		FAnimMontageInstance* Active = MutableInstance->GetActiveInstanceForMontage(Montage);
		const bool bOwnsActive = Active && Active->GetInstanceID() == OwnedMontageInstanceId;
		if (bMatched && bRelevantLastFrame)
		{
			if (MontageRequestId != RequestId)
			{
				if (MutableInstance->Montage_Play(Montage, AutomaticPlayRate, EMontagePlayReturnType::MontageLength, 0.0f, false) > 0.0f)
				{
					Active = MutableInstance->GetActiveInstanceForMontage(Montage);
					OwnedMontageInstanceId = Active ? Active->GetInstanceID() : INDEX_NONE;
				}
				MontageRequestId = RequestId;
			}
			else if (bOwnsActive) MutableInstance->Montage_SetPlayRate(Montage, AutomaticPlayRate);
		}
		else
		{
			if (bOwnsActive) MutableInstance->Montage_Stop(Montage->BlendOut.GetBlendTime(), Montage);
			MontageRequestId = INDEX_NONE;
			OwnedMontageInstanceId = INDEX_NONE;
		}
	}
	bRelevantLastFrame = false;
}

void FLxAnimNode_ActionPlayer::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);
	bRelevantLastFrame = Context.GetFinalBlendWeight() > ZERO_ANIMWEIGHT_THRESH;
	if (Cast<UAnimMontage>(Animation))
	{
		MontageSlot.Update_AnyThread(Context);
	}
	else if (bMatched)
	{
		const bool bRestart = !bWasMatched || LastRequestId != RequestId;
		if (Cast<UBlendSpace>(Animation))
		{
			BlendPlayer.SetPosition(BlendPosition);
			BlendPlayer.SetPlayRate(AutomaticPlayRate);
			BlendPlayer.SetLoop(bLoop);
			if (bRestart) BlendPlayer.Initialize_AnyThread(FAnimationInitializeContext(Context.AnimInstanceProxy));
			BlendPlayer.Update_AnyThread(Context);
		}
		else
		{
			SequencePlayer.SetPlayRate(AutomaticPlayRate);
			SequencePlayer.SetLoopAnimation(bLoop);
			if (bRestart) SequencePlayer.SetAccumulatedTime(0.0f);
			SequencePlayer.Update_AnyThread(Context);
		}
		bHasPlayed = true;
	}
	bWasMatched = bMatched;
	LastRequestId = RequestId;
}

void FLxAnimNode_ActionPlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	if (Cast<UAnimMontage>(Animation)) MontageSlot.Evaluate_AnyThread(Output);
	else if (!bHasPlayed) Output.ResetToRefPose();
	else if (Cast<UBlendSpace>(Animation)) BlendPlayer.Evaluate_AnyThread(Output);
	else SequencePlayer.Evaluate_AnyThread(Output);
}

void FLxAnimNode_ActionPlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(FString::Printf(TEXT("动作匹配=%s 自动速率=%.2f"), bMatched ? TEXT("是") : TEXT("否"), AutomaticPlayRate));
}
