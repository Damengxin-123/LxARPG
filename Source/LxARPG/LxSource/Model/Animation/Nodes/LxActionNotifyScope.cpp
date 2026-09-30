#include "LxActionNotifyScope.h"
#include "LxAnimNode_ActionPlayer.h"
#include "Animation/AnimInstance.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"

IMPLEMENT_NOTIFY_CONTEXT_INTERFACE(FLxDisabledActionNotifyContext)
IMPLEMENT_ANIMGRAPH_MESSAGE(FLxActionNotifyScope)
IMPLEMENT_NOTIFY_CONTEXT_INTERFACE(FLxActionEventContext)
IMPLEMENT_ANIMGRAPH_MESSAGE(FLxActionEventScope)

TUniquePtr<const UE::Anim::IAnimNotifyEventContextDataInterface> FLxActionEventScope::MakeUniqueEventContextData() const
{
	return MakeUnique<const FLxActionEventContext>(Event);
}

TUniquePtr<const UE::Anim::IAnimNotifyEventContextDataInterface> FLxActionNotifyScope::MakeUniqueEventContextData() const
{
	return MakeUnique<const FLxDisabledActionNotifyContext>();
}

void FLxActionAnimInstanceProxy::PostUpdate(UAnimInstance* Instance) const
{
	FAnimInstanceProxy::PostUpdate(Instance);
	ULxAnimInstanceBase* ActionInstance = Cast<ULxAnimInstanceBase>(Instance);
	Instance->NotifyQueue.AnimNotifies.RemoveAll([ActionInstance](const FAnimNotifyEventReference& Event)
	{
		if (Event.GetContextData<FLxDisabledActionNotifyContext>()) return true;
		const auto* Montage = Event.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
		return Montage && ActionInstance && ActionInstance->IsActionMontageNotifyDisabled(Montage->MontageInstanceID);
	});
	if (ActionInstance)
	{
		// 只收集，等待姿势刷新完成后再向游戏逻辑发送，避免通知回调重入动画更新。
		for (const FAnimNotifyEventReference& Event : Instance->NotifyQueue.AnimNotifies)
			ActionInstance->CollectAnimationNotify(Event);
		for (FLxAnimNode_ActionPlayer* Player : UpdatedPlayers)
		{
			FLxCharacterAnimationEvent EndEvent;
			if (Player->CollectPlaybackEnd(Instance, EndEvent)) ActionInstance->CollectPlaybackEvent(EndEvent);
		}
		ActionInstance->PruneActionMontageNotifyFilters();
	}
}

void FLxActionAnimInstanceProxy::PreUpdate(UAnimInstance* Instance, float DeltaSeconds)
{
	UpdatedPlayers.Reset();
	FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
}
