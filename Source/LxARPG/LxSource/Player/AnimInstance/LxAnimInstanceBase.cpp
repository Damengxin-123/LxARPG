// Fill out your copyright notice in the Description page of Project Settings.


#include "LxAnimInstanceBase.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Components/SkeletalMeshComponent.h"
#include "LxARPG/LxSource/Model/Animation/Nodes/LxAnimNotify_ActionEvent.h"
#include "LxARPG/LxSource/Model/Animation/Nodes/LxActionNotifyScope.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"

#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

FAnimInstanceProxy* ULxAnimInstanceBase::CreateAnimInstanceProxy()
{
	return new FLxActionAnimInstanceProxy(this);
}

void ULxAnimInstanceBase::RegisterActionMontageSource(int32 InstanceId, const FLxCharacterAnimationEvent& Source)
{
	if (InstanceId != INDEX_NONE) MontageSources.Add(InstanceId, Source);
}

void ULxAnimInstanceBase::CollectAnimationNotify(const FAnimNotifyEventReference& Reference)
{
	const FAnimNotifyEvent* Notify = Reference.GetNotify();
	// 状态通知每帧都会入队，不能当作单次释放事件。
	if (!Notify || Notify->NotifyStateClass) return;
	FLxCharacterAnimationEvent Event;
	if (const auto* Montage = Reference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>())
	{
		if (const auto* Source = MontageSources.Find(Montage->MontageInstanceID)) Event = *Source;
	}
	else if (const auto* Source = Reference.GetContextData<FLxActionEventContext>()) Event = Source->Event;
	const ULxAnimNotify_ActionEvent* ActionNotify = Cast<ULxAnimNotify_ActionEvent>(Notify->Notify);
	Event.NotifyName = ActionNotify ? ActionNotify->EventName : Notify->NotifyName;
	if (!Event.NotifyName.IsNone()) QueueAnimationEvent(Event);
}

void ULxAnimInstanceBase::CollectActionBranchingPoint(FName NotifyName, int32 MontageInstanceId)
{
	if (IsActionMontageNotifyDisabled(MontageInstanceId)) return;
	if (const auto* Source = MontageSources.Find(MontageInstanceId))
	{
		FLxCharacterAnimationEvent Event = *Source;
		Event.NotifyName = NotifyName;
		QueueAnimationEvent(Event);
	}
}

void ULxAnimInstanceBase::QueueAnimationEvent(const FLxCharacterAnimationEvent& Event)
{
	ULxAnimInstanceBase* MainInstance = GetSkelMeshComponent()
		? Cast<ULxAnimInstanceBase>(GetSkelMeshComponent()->GetAnimInstance()) : nullptr;
	if (MainInstance && MainInstance != this) MainInstance->QueueAnimationEvent(Event);
	else PendingAnimationEvents.Add(Event);
}

void ULxAnimInstanceBase::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
	TArray<FLxCharacterAnimationEvent> Events = MoveTemp(PendingAnimationEvents);
	PendingAnimationEvents.Reset();
	// 分支点先于普通通知入队；跨越多个标记的一帧须先执行释放，再处理结束。
	Events.StableSort([](const FLxCharacterAnimationEvent& Left, const FLxCharacterAnimationEvent& Right)
	{
		return (Left.NotifyName == FLxCharacterAnimationEvent::FinishName() ? 1 : 0)
			< (Right.NotifyName == FLxCharacterAnimationEvent::FinishName() ? 1 : 0);
	});
	for (const FLxCharacterAnimationEvent& Event : Events)
	{
		OnAnimationEvent.Broadcast(Event);
		ReceiveAnimationEvent(Event);
	}
}

void ULxAnimInstanceBase::PruneActionMontageNotifyFilters()
{
	for (auto It = MontageSources.CreateIterator(); It; ++It)
		if (!GetMontageInstanceForID(It.Key())) It.RemoveCurrent();
	for (auto It = DisabledActionMontageInstances.CreateIterator(); It; ++It)
		if (!GetMontageInstanceForID(*It)) It.RemoveCurrent();
}

void ULxAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	DisabledActionMontageInstances.Reset();
	PendingAnimationEvents.Reset();
	MontageSources.Reset();
	
	if (APawn* Pawn = TryGetPawnOwner())
	{
		m_pCharacter = Cast<ALxBaseCharacter>(Pawn);
	}
	// 构建动画类型与动画资产的运行时映射。
	AnimationAssetMap.Empty();
	for (const FLxCharacterAnimationAssetConfig& Config : AnimationAssetConfigs)
	{
		if (Config.AnimationAsset)
		{
			AnimationAssetMap.Add(Config.AnimationType, Config.AnimationAsset);
		}
	}

	CurrentBaseAnimationAsset = GetConfiguredAnimationAsset(CurrentBaseAnimationType);
	if (!CurrentBaseAnimationAsset)
	{
		CurrentBaseAnimationAsset = DefaultAnimationAsset;
	}
	CurrentActionAnimationAsset = nullptr;
	bShouldBlendActionAnimation = false;
}

void ULxAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!m_pCharacter)
	{
		if (APawn* Pawn = TryGetPawnOwner())
		{
			m_pCharacter = Cast<ALxBaseCharacter>(Pawn);
		}
	}
	if (m_pCharacter)
	{
		if (const ULxCharacterBehaviorControlComponent* Behavior = m_pCharacter->GetCharacterBehaviorControlComponent())
		{
			CurrentMotionType = Behavior->GetCurrentMotionType();
			if (CurrentActionAnimationType == ELxCharacterMotionType::None)
				AttackMotionType = CurrentMotionType == ELxCharacterMotionType::Defend
					? ELxCharacterMotionType::Defend : ELxCharacterMotionType::None;
		}
		bShouldBlendActionAnimation = AttackMotionType != ELxCharacterMotionType::None;
		// 同步角色复制状态，保证动画图表能够在生命值归零后读取 Dead（死亡状态）。
		m_nCharacterState = m_pCharacter->GetCurrentState();
	}
}

void ULxAnimInstanceBase::ApplyBaseAnimationSignal(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	UpdateBaseAnimationPlayback(InAnimationSignal);
}

void ULxAnimInstanceBase::ApplyActionAnimationSignal(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	UpdateActionAnimationPlayback(InAnimationSignal);
}

void ULxAnimInstanceBase::UpdateBaseAnimationPlayback(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	CurrentBaseAnimationSignal = InAnimationSignal;
	CurrentBaseAnimationType = InAnimationSignal.AnimationType;
	BaseMotionType = InAnimationSignal.AnimationType;
	CurrentBaseAnimationPlayRate = InAnimationSignal.PlayRate;
	bCurrentBaseAnimationLoop = InAnimationSignal.bLoop;
	CurrentBaseAnimationAsset = GetConfiguredAnimationAsset(CurrentBaseAnimationType);
	if (!CurrentBaseAnimationAsset)
	{
		CurrentBaseAnimationAsset = DefaultAnimationAsset;
	}
}

void ULxAnimInstanceBase::UpdateActionAnimationPlayback(const FLxCharacterAnimationSignal& InAnimationSignal)
{
	ActionAnimationPlayRequestId = ActionAnimationPlayRequestId == MAX_int32
		? 1
		: ActionAnimationPlayRequestId + 1;
	CurrentActionAnimationSignal = InAnimationSignal;
	CurrentActionAnimationType = InAnimationSignal.AnimationType;
	AttackMotionType = InAnimationSignal.AnimationType;
	CurrentAttackSkillId = InAnimationSignal.SkillId;
	CurrentActionAnimationPlayRate = InAnimationSignal.PlayRate;
	bCurrentActionAnimationLoop = InAnimationSignal.bLoop;
	CurrentActionAnimationAsset = CurrentActionAnimationType == ELxCharacterMotionType::None
		? nullptr
		: GetConfiguredAnimationAsset(CurrentActionAnimationType);
	bShouldBlendActionAnimation = AttackMotionType != ELxCharacterMotionType::None;
}

UAnimationAsset* ULxAnimInstanceBase::GetConfiguredAnimationAsset(ELxCharacterMotionType InAnimationType) const
{
	if (const TObjectPtr<UAnimationAsset>* AnimationAsset = AnimationAssetMap.Find(InAnimationType))
	{
		return AnimationAsset->Get();
	}

	// 初始化前也允许蓝图查询配置，避免缓存尚未建立时错误返回空。
	for (const FLxCharacterAnimationAssetConfig& Config : AnimationAssetConfigs)
	{
		if (Config.AnimationType == InAnimationType && Config.AnimationAsset)
		{
			return Config.AnimationAsset.Get();
		}
	}

	return nullptr;
}

bool ULxAnimInstanceBase::MatchesMotion(bool bAttackChannel, ELxCharacterMotionType MotionType, FGameplayTag SkillId) const
{
	return MotionType != ELxCharacterMotionType::None
		&& MotionType == (bAttackChannel ? AttackMotionType : BaseMotionType)
		&& (!bAttackChannel || !SkillId.IsValid() || SkillId == CurrentAttackSkillId);
}
