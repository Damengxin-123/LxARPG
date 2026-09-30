#include "LxAnimNode_ActionPlayer.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "LxActionNotifyScope.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"

FLxActionPoseState FLxAnimNode_ActionPlayer::GetActionState() const
{
	FLxActionPoseState State;
	State.bActive = bMatched;
	State.bUseBoneBlend = bUseBoneBlend;
	State.Priority = SkillId.IsValid() ? 1 : 0;
	State.Key = HashCombine(HashCombine(PointerHash(this), GetTypeHash(RequestId)), GetTypeHash(bUseBoneBlend));
	return State;
}

void FLxAnimNode_ActionPlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	CompletedCastId.Invalidate();
	NotifySource = FLxCharacterAnimationEvent();
	bWasMatched = false;
	bHasPlayed = false;
	bRelevantLastFrame = false;
	bCachePlaybackLastFrame = false;
	bContinueFromCache = false;
	bRestartFromCache = false;
	MontageRequestId = INDEX_NONE;
	OwnedMontageInstanceId = INDEX_NONE;
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
	if (Instance && bMatched)
	{
		AutomaticPlayRate = bAttackChannel ? Instance->CurrentActionAnimationPlayRate : Instance->CurrentBaseAnimationPlayRate;
		if (!FMath::IsFinite(AutomaticPlayRate)) AutomaticPlayRate = 1.0f;
		bLoop = bAttackChannel ? (MotionType == ELxCharacterMotionType::Defend || Instance->bCurrentActionAnimationLoop) : Instance->bCurrentBaseAnimationLoop;
		RequestId = bAttackChannel ? Instance->ActionAnimationPlayRequestId : 0;
		NotifySource.bActionChannel = bAttackChannel;
		NotifySource.SkillId = bAttackChannel ? Instance->CurrentActionAnimationSignal.SkillId : FGameplayTag();
		NotifySource.CastId = bAttackChannel ? Instance->CurrentActionAnimationSignal.CastId : FGuid();
	}
	if (UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		// 原生蒙太奇操作仅允许在游戏线程执行；图首次相关后一帧开始播放。
		UAnimInstance* MutableInstance = const_cast<UAnimInstance*>(InAnimInstance);
		if (!bReceiveAnimationNotifies)
		{
			if (!SilentMontage)
			{
				// 仅复制蒙太奇描述；底层序列继续共享，避免复制压缩动画数据。
				SilentMontage = DuplicateObject<UAnimMontage>(Montage, MutableInstance,
					MakeUniqueObjectName(MutableInstance, UAnimMontage::StaticClass(), TEXT("静默动作")));
				SilentMontage->SetFlags(RF_Transient);
				// 编辑器复制会从数据模型重建时长，动态蒙太奇必须保留实际组合时长。
				SilentMontage->SetCompositeLength(Montage->GetPlayLength());
				SilentMontage->Notifies.Reset();
				SilentMontage->RefreshCacheData();
			}
			Montage = SilentMontage;
		}
		FAnimMontageInstance* Active = MutableInstance->GetActiveInstanceForMontage(Montage);
		const bool bOwnsActive = Active && Active->GetInstanceID() == OwnedMontageInstanceId;
		if ((bMatched || bCachePlaybackLastFrame) && bRelevantLastFrame)
		{
			if (MontageRequestId != RequestId)
			{
				// 同动作新请求替换自己拥有的旧实例，避免两个实例重复通知或争用插槽。
				if (bOwnsActive) MutableInstance->Montage_Stop(0.0f, Montage);
				if (MutableInstance->Montage_Play(Montage, AutomaticPlayRate, EMontagePlayReturnType::MontageLength, 0.0f, false) > 0.0f)
				{
					Active = MutableInstance->GetActiveInstanceForMontage(Montage);
					OwnedMontageInstanceId = Active ? Active->GetInstanceID() : INDEX_NONE;
					if (ULxAnimInstanceBase* ActionInstance = Cast<ULxAnimInstanceBase>(MutableInstance))
						ActionInstance->RegisterActionMontageSource(OwnedMontageInstanceId, NotifySource);
					// 缓存保持非循环蒙太奇的末帧，直到信号或上游选择发生变化。
					if (Active && bCachePlaybackLastFrame) Active->bEnableAutoBlendOut = false;
					if (!bReceiveAnimationNotifies && OwnedMontageInstanceId != INDEX_NONE)
						if (ULxAnimInstanceBase* ActionInstance = Cast<ULxAnimInstanceBase>(MutableInstance))
							ActionInstance->DisableActionMontageNotifies(OwnedMontageInstanceId);
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
	UE::Anim::TScopedGraphMessage<FLxActionEventScope> EventScope(Context, NotifySource);
	if (bReceiveAnimationNotifies) UpdatePlayer(Context);
	else
	{
		UE::Anim::TScopedGraphMessage<FLxActionNotifyScope> Scope(Context);
		UpdatePlayer(Context);
	}
	bContinueFromCache = false;
	bRestartFromCache = false;
}

void FLxAnimNode_ActionPlayer::UpdatePlayer(const FAnimationUpdateContext& Context)
{
	if (Context.AnimInstanceProxy->GetAnimInstanceObject()->IsA<ULxAnimInstanceBase>())
		static_cast<FLxActionAnimInstanceProxy*>(Context.AnimInstanceProxy)->RegisterActionPlayer(this);
	bRelevantLastFrame = Context.GetFinalBlendWeight() > ZERO_ANIMWEIGHT_THRESH;
	bCachePlaybackLastFrame = bContinueFromCache;
	const bool bPlaying = bMatched || (bContinueFromCache && bHasPlayed);
	if (Cast<UAnimMontage>(Animation))
	{
		MontageSlot.Update_AnyThread(Context);
	}
	else if (bPlaying)
	{
		const bool bRestart = bRestartFromCache || !bWasMatched || LastRequestId != RequestId;
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
	bWasMatched = bPlaying;
	LastRequestId = RequestId;
}

void FLxAnimNode_ActionPlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	if (Cast<UAnimMontage>(Animation)) MontageSlot.Evaluate_AnyThread(Output);
	else if (!bHasPlayed) Output.ResetToRefPose();
	else if (Cast<UBlendSpace>(Animation)) BlendPlayer.Evaluate_AnyThread(Output);
	else SequencePlayer.Evaluate_AnyThread(Output);
}

bool FLxAnimNode_ActionPlayer::CollectPlaybackEnd(UAnimInstance* Instance, FLxCharacterAnimationEvent& OutEvent)
{
	if (!bAttackChannel || !NotifySource.CastId.IsValid() || CompletedCastId == NotifySource.CastId
		|| bLoop || !bRelevantLastFrame) return false;
	float Position = 0.0f;
	float Length = 0.0f;
	if (Cast<UAnimMontage>(Animation))
	{
		const FAnimMontageInstance* Montage = Instance->GetMontageInstanceForID(OwnedMontageInstanceId);
		if (!Montage || !Montage->Montage) return false;
		Position = Montage->GetPosition();
		Length = Montage->Montage->GetPlayLength();
	}
	else if (Cast<UBlendSpace>(Animation))
	{
		Position = BlendPlayer.GetCurrentAssetTime();
		Length = BlendPlayer.GetCurrentAssetLength();
	}
	else if (bHasPlayed && SequencePlayer.GetSequence())
	{
		Position = SequencePlayer.GetAccumulatedTime();
		Length = SequencePlayer.GetSequence()->GetPlayLength();
	}
	if (Length <= 0.0f || Position < Length - KINDA_SMALL_NUMBER) return false;
	CompletedCastId = NotifySource.CastId;
	OutEvent = NotifySource;
	OutEvent.NotifyName = FLxCharacterAnimationEvent::FinishName();
	return true;
}

void FLxAnimNode_ActionPlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(FString::Printf(TEXT("动作匹配=%s 自动速率=%.2f"), bMatched ? TEXT("是") : TEXT("否"), AutomaticPlayRate));
}
