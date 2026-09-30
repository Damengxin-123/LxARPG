#include "LxSkillCastComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerAimModule.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxSkill.h"

ULxSkillCastModule::ULxSkillCastModule()
{
}

void ULxSkillCastModule::InitializeModule(ULxCharacterCombatComponent* InOwnerComponent)
{
	Super::InitializeModule(InOwnerComponent);
	CurrentCastContext = MakeSkillCastContext(this);
}

void ULxSkillCastModule::ShutdownModule()
{
	CancelCurrentSkillRelease();
	EndSustainedAimTracking();
	Super::ShutdownModule();
}

FLxSkillCastContext ULxSkillCastModule::MakeSkillCastContext(UObject* SourceObject, AActor* TargetActor,
	FVector AimLocation, bool bHasAimLocation, FVector AimDirection, bool bHasAimDirection) const
{
	FLxSkillCastContext CastContext;
	CastContext.WorldContextObject = const_cast<ULxSkillCastModule*>(this);
	CastContext.CasterActor = GetOwner();
	CastContext.TargetActor = TargetActor;
	CastContext.SourceObject = SourceObject;
	CastContext.AimLocation = AimLocation;
	CastContext.bHasAimLocation = bHasAimLocation;
	CastContext.AimDirection = bHasAimDirection ? AimDirection.GetSafeNormal() : AimDirection;
	CastContext.bHasAimDirection = bHasAimDirection;

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		CastContext.InstigatorController = OwnerPawn->GetController();
	}
	else if (const AActor* OwnerActor = GetOwner())
	{
		CastContext.InstigatorController = OwnerActor->GetInstigatorController();
	}

	if (const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner()))
	{
		CastContext.SpawnTransform = OwnerCharacter->GetSkillReleaseAnchorTransform();
		CastContext.bOverrideSpawnTransform = false;
	}
	else if (const AActor* OwnerActor = GetOwner())
	{
		CastContext.SpawnTransform = OwnerActor->GetActorTransform();
	}

	return CastContext;
}

bool ULxSkillCastModule::InitializeSkillForCast(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill)
	{
		return false;
	}

	CurrentCastContext = NormalizeCastContext(InCastContext);
	InSkill->OnSkillHitEntriesReady.RemoveAll(this);
	InSkill->OnSkillHitEntriesReady.AddUObject(this, &ULxSkillCastModule::HandleSkillHitEntriesReady);
	InSkill->OnPersistentSkillHitEntriesReady.RemoveAll(this);
	InSkill->OnPersistentSkillHitEntriesReady.AddUObject(this, &ULxSkillCastModule::HandlePersistentSkillHitEntriesReady);
	InSkill->OnSkillEffectsRemoved.RemoveAll(this);
	InSkill->OnSkillEffectsRemoved.AddUObject(this, &ULxSkillCastModule::HandleSkillEffectsRemoved);
	InSkill->PrepareSkillForCast(CurrentCastContext);
	return true;
}

bool ULxSkillCastModule::ReleaseSkillDirectly(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || OwnerComponent == nullptr || !OwnerComponent->CanStartSkillCast()
		|| !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	SetSkillCastState(ELxSkillCastState::DirectReleasing);
	CurrentCastingSkill = InSkill;
	if (!InSkill->TryBeginDirectSkillReleaseTiming())
	{
		ResetSkillCastState();
		return false;
	}

	BeginAnimationSkillRelease(InSkill, ELxPendingSkillReleaseExecution::Direct);
	return true;
}

bool ULxSkillCastModule::StartSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || OwnerComponent == nullptr || !OwnerComponent->CanStartSkillCast()
		|| !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	SetSkillCastState(ELxSkillCastState::Charging);
	CurrentCastingSkill = InSkill;
	ChargingSkill = InSkill;
	ChargingSkillItem = nullptr;
	if (!InSkill->BeginSkillCharge())
	{
		ResetSkillCastState();
		return false;
	}

	return true;
}

bool ULxSkillCastModule::EndSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	ULxSkill* SkillToEnd = InSkill ? InSkill : ChargingSkill.Get();
	if (SkillCastState != ELxSkillCastState::Charging || SkillToEnd == nullptr
		|| SkillToEnd != CurrentCastingSkill || !InitializeSkillForCast(SkillToEnd, InCastContext)
		|| !SkillToEnd->TryBeginChargeSkillReleaseTiming())
	{
		return false;
	}

	ChargingSkill = nullptr;
	ChargingSkillItem = nullptr;
	SetSkillCastState(ELxSkillCastState::DirectReleasing);
	BeginAnimationSkillRelease(SkillToEnd, ELxPendingSkillReleaseExecution::Charge);
	return true;
}

bool ULxSkillCastModule::StartSustainedRelease(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || OwnerComponent == nullptr || !OwnerComponent->CanStartSkillCast()
		|| !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	SetSkillCastState(ELxSkillCastState::SustainedReleasing);
	CurrentCastingSkill = InSkill;
	SustainedSkill = InSkill;
	if (!InSkill->TryBeginSustainedSkillReleaseTiming())
	{
		ResetSkillCastState();
		return false;
	}

	BeginAnimationSkillRelease(InSkill, ELxPendingSkillReleaseExecution::Sustained);
	BeginSustainedAimTracking();
	return true;
}

bool ULxSkillCastModule::StopSustainedRelease(ULxSkill* InSkill)
{
	ULxSkill* SkillToStop = InSkill ? InSkill : SustainedSkill.Get();
	if (SkillCastState != ELxSkillCastState::SustainedReleasing || !SkillToStop
		|| SkillToStop != CurrentCastingSkill || SkillToStop != SustainedSkill)
	{
		return false;
	}

	ClearAnimationSkillRelease(true);
	const bool bStopped = SkillToStop->TryStopSustainedRelease();
	if (OwnerComponent) OwnerComponent->RequestStopSkillActionAnimation();
	EndSustainedAimTracking();
	ResetSkillCastState();
	return bStopped;
}

bool ULxSkillCastModule::CancelSustainedRelease(ULxSkill* InSkill)
{
	ULxSkill* SkillToCancel = InSkill ? InSkill : SustainedSkill.Get();
	if (SkillCastState != ELxSkillCastState::SustainedReleasing || !SkillToCancel
		|| SkillToCancel != CurrentCastingSkill || SkillToCancel != SustainedSkill)
	{
		return false;
	}

	ClearAnimationSkillRelease(true);
	const bool bCancelled = SkillToCancel->TryCancelSustainedRelease();
	if (OwnerComponent) OwnerComponent->RequestStopSkillActionAnimation();
	EndSustainedAimTracking();
	ResetSkillCastState();
	return bCancelled;
}

bool ULxSkillCastModule::CancelCurrentSkillRelease()
{
	if (IsSkillCastIdle())
	{
		return false;
	}

	ULxSkill* SkillToCancel = CurrentCastingSkill.Get();
	bool bCancelled = false;
	if (SkillToCancel)
	{
		ClearAnimationSkillRelease(true);
		bCancelled = SkillCastState == ELxSkillCastState::SustainedReleasing
			? SkillToCancel->TryCancelSustainedRelease()
			: SkillToCancel->TryCancelSkillRelease();
		if (OwnerComponent) OwnerComponent->RequestStopSkillActionAnimation();
	}

	EndSustainedAimTracking();
	ResetSkillCastState();
	return bCancelled;
}

bool ULxSkillCastModule::FinishCurrentSkillRelease(ULxSkill* InSkill)
{
	if (IsSkillCastIdle() || InSkill == nullptr || InSkill != CurrentCastingSkill
		|| PendingSkillReleaseExecution != ELxPendingSkillReleaseExecution::None)
	{
		return false;
	}

	EndSustainedAimTracking();
	ResetSkillCastState();
	return true;
}

bool ULxSkillCastModule::HandleSkillReleaseInput(ULxSkill* InSkill, ELxSkillReleaseInputState InInputState, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill || InInputState == ELxSkillReleaseInputState::None)
	{
		return false;
	}

	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		const FGameplayTag SkillItemIDTag = ResolveSkillItemIDTag(InSkill);
		if (!SkillItemIDTag.IsValid())
		{
			return false;
		}

		const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext);
		if (OwnerComponent == nullptr) return false;
		OwnerComponent->ServerHandleSkillItemReleaseInput(SkillItemIDTag, InInputState, SkillContext.TargetActor,
			SkillContext.AimLocation, SkillContext.bHasAimLocation,
			SkillContext.AimDirection, SkillContext.bHasAimDirection);
		return true;
	}

	return HandleSkillReleaseInputAuthority(InSkill, InInputState, InCastContext);
}

bool ULxSkillCastModule::HandleSkillReleaseInputAuthority(ULxSkill* InSkill,
	ELxSkillReleaseInputState InInputState, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill || InInputState == ELxSkillReleaseInputState::None)
	{
		return false;
	}

	const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext);
	switch (InSkill->GetSkillReleaseType())
	{
	case ELxSkillReleaseType::DirectRelease:
		if (InInputState == InSkill->GetDirectReleaseInputState())
		{
			if (InSkill->ShouldStopPersistentSkillOnRepeatedRelease() && InSkill->IsPersistentSkillUnitGroupActive())
			{
				return InSkill->TryStopPersistentSkillUnitGroup();
			}
			return ReleaseSkillDirectly(InSkill, SkillContext);
		}
		break;
	case ELxSkillReleaseType::ChargeRelease:
		if (InInputState == ELxSkillReleaseInputState::Start)
		{
			return StartSkillCharge(InSkill, SkillContext);
		}
		if (InInputState == ELxSkillReleaseInputState::End)
		{
			return EndSkillCharge(InSkill, SkillContext);
		}
		if (InInputState == ELxSkillReleaseInputState::Cancel && ChargingSkill == InSkill)
		{
			return CancelCurrentSkillRelease();
		}
		break;
	case ELxSkillReleaseType::SustainedRelease:
		if (InInputState == ELxSkillReleaseInputState::Start)
		{
			return StartSustainedRelease(InSkill, SkillContext);
		}
		if (InInputState == ELxSkillReleaseInputState::End)
		{
			return StopSustainedRelease(InSkill);
		}
		if (InInputState == ELxSkillReleaseInputState::Cancel)
		{
			return CancelSustainedRelease(InSkill);
		}
		break;
	default:
		break;
	}

	return false;
}

void ULxSkillCastModule::HandleSkillItemReleaseInputFromServer(FGameplayTag InSkillItemIDTag,
	ELxSkillReleaseInputState InInputState, AActor* InTargetActor, FVector_NetQuantize InAimLocation,
	bool bInHasAimLocation, FVector_NetQuantizeNormal InAimDirection, bool bInHasAimDirection)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	ULxSkillBackpackModule* SkillBackpack = OwnerCharacter ? OwnerCharacter->GetSkillBackpackComponent() : nullptr;
	ULxSkillItem* SkillItem = SkillBackpack ? SkillBackpack->FindSkillItemByTagID(InSkillItemIDTag) : nullptr;
	ULxSkill* Skill = SkillItem ? SkillItem->GetOrCreateSkillObject() : nullptr;
	if (!Skill)
	{
		return;
	}

	FLxSkillCastContext ServerContext = MakeSkillCastContext(SkillItem, InTargetActor,
		InAimLocation, bInHasAimLocation, InAimDirection, bInHasAimDirection);
	HandleSkillReleaseInputAuthority(Skill, InInputState, ServerContext);
}

void ULxSkillCastModule::PlaySkillActionAnimation(FGuid InCastId, FGameplayTag InSkillId, ELxCharacterMotionType InMotionType)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	ULxCharacterBehaviorControlComponent* BehaviorControlComponent = OwnerCharacter
		? OwnerCharacter->GetCharacterBehaviorControlComponent()
		: nullptr;
	if (!BehaviorControlComponent)
	{
		return;
	}

	FLxCharacterMotionSignal ActionMotionSignal;
	ActionMotionSignal.MotionType = InMotionType;
	ActionMotionSignal.SkillId = InSkillId;
	// 使用动画自身时长；加速减速由动画处理组件统一转换。
	ActionMotionSignal.MotionSpeed = 1.0f;
	ActionMotionSignal.CastId = InCastId;
	ActionMotionSignal.bLoop = ActionMotionSignal.MotionType == ELxCharacterMotionType::Defend;
	BehaviorControlComponent->SendActionAnimationMotionSignal(ActionMotionSignal);
}

void ULxSkillCastModule::StopSkillActionAnimation()
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	ULxCharacterBehaviorControlComponent* BehaviorControlComponent = OwnerCharacter
		? OwnerCharacter->GetCharacterBehaviorControlComponent()
		: nullptr;
	if (!BehaviorControlComponent)
	{
		return;
	}

	FLxCharacterMotionSignal ActionMotionSignal;
	ActionMotionSignal.MotionType = ELxCharacterMotionType::None;
	ActionMotionSignal.MotionSpeed = 0.0f;
	ActionMotionSignal.bLoop = ActionMotionSignal.MotionType == ELxCharacterMotionType::Defend;
	BehaviorControlComponent->SendActionAnimationMotionSignal(ActionMotionSignal);
}

void ULxSkillCastModule::BeginAnimationSkillRelease(ULxSkill* InSkill,
	ELxPendingSkillReleaseExecution InExecutionType)
{
	if (!InSkill || !GetWorld())
	{
		if (InSkill)
		{
			InSkill->CancelSkillReleaseTiming();
		}
		ResetSkillCastState();
		return;
	}

	ClearAnimationSkillRelease(false);
	PendingSkillReleaseExecution = InExecutionType;
	ActiveCastId = FGuid::NewGuid();
	ActiveSkillId = ResolveSkillItemIDTag(InSkill);
	bReleaseExecuted = false;
	ELxCharacterMotionType MotionType = InSkill->AnimationMotionType;
	if (const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner()))
		if (const ULxCharacterBehaviorControlComponent* Behavior = Character->GetCharacterBehaviorControlComponent())
		{
			const ELxCharacterMotionType BehaviorType = Behavior->GetCurrentMotionType();
			if (BehaviorType == ELxCharacterMotionType::Attack || BehaviorType == ELxCharacterMotionType::RangedAttack
				|| BehaviorType == ELxCharacterMotionType::Defend) MotionType = BehaviorType;
		}
	if (OwnerComponent) OwnerComponent->RequestPlaySkillActionAnimation(ActiveCastId, ActiveSkillId, MotionType);

}

void ULxSkillCastModule::ExecuteAnimationSkillRelease()
{
	ULxSkill* Skill = CurrentCastingSkill.Get();
	if (!Skill)
	{
		ClearAnimationSkillRelease(false);
		ResetSkillCastState();
		return;
	}

	// 先消费执行点，技能蓝图回调即使重入也不能再次创建实体。
	if (bReleaseExecuted) return;
	bReleaseExecuted = true;
	switch (PendingSkillReleaseExecution)
	{
	case ELxPendingSkillReleaseExecution::Direct:
		Skill->ExecuteDirectSkillRelease();
		break;
	case ELxPendingSkillReleaseExecution::Charge:
		Skill->ExecuteChargeSkillRelease();
		break;
	case ELxPendingSkillReleaseExecution::Sustained:
		Skill->ExecuteSustainedSkillRelease();
		break;
	default:
		break;
	}
}

void ULxSkillCastModule::HandleAnimationEvent(const FLxCharacterAnimationEvent& Event)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !CurrentCastingSkill
		|| !Event.bActionChannel || !ActiveCastId.IsValid() || Event.CastId != ActiveCastId
		|| Event.SkillId != ActiveSkillId || PendingSkillReleaseExecution == ELxPendingSkillReleaseExecution::None)
	{
		return;
	}
	if (Event.NotifyName == FLxCharacterAnimationEvent::ReleaseName()) ExecuteAnimationSkillRelease();
	else if (Event.NotifyName == FLxCharacterAnimationEvent::FinishName()) CompleteAnimationSkillRelease();
}

void ULxSkillCastModule::CompleteAnimationSkillRelease()
{
	if (!bReleaseExecuted)
	{
		CancelCurrentSkillRelease();
		return;
	}
	ActiveCastId.Invalidate();
	ULxSkill* Skill = CurrentCastingSkill.Get();
	const ELxPendingSkillReleaseExecution CompletedExecution = PendingSkillReleaseExecution;
	PendingSkillReleaseExecution = ELxPendingSkillReleaseExecution::None;
	if (!Skill)
	{
		ResetSkillCastState();
		return;
	}

	Skill->CompleteSkillReleaseTiming();
	if (OwnerComponent) OwnerComponent->RequestStopSkillActionAnimation();
	if (CompletedExecution != ELxPendingSkillReleaseExecution::Sustained
		&& !Skill->ShouldHoldReleaseStateUntilExplicitFinish())
	{
		ResetSkillCastState();
	}
}

void ULxSkillCastModule::ClearAnimationSkillRelease(bool bCancelSkillTiming)
{
	// 已经创建过实体的施法即使被打断也必须进入冷却，防止取消后摇绕过冷却。
	if (bCancelSkillTiming && bReleaseExecuted && CurrentCastingSkill)
		CurrentCastingSkill->CompleteSkillReleaseTiming();
	ActiveCastId.Invalidate();
	ActiveSkillId = FGameplayTag();
	bReleaseExecuted = false;

	if (bCancelSkillTiming && CurrentCastingSkill)
	{
		CurrentCastingSkill->CancelSkillReleaseTiming();
	}
	PendingSkillReleaseExecution = ELxPendingSkillReleaseExecution::None;
}

FGameplayTag ULxSkillCastModule::ResolveSkillItemIDTag(const ULxSkill* InSkill) const
{
	ULxSkillItem* SkillItem = InSkill ? Cast<ULxSkillItem>(InSkill->GetOuter()) : nullptr;
	return SkillItem && SkillItem->ItemIsValid() ? SkillItem->ItemIDTag() : FGameplayTag();
}

bool ULxSkillCastModule::ReleaseSkillItemDirectly(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
{
	if (!InSkillItem || !InSkillItem->ItemIsValid())
	{
		return false;
	}

	ULxSkill* Skill = InSkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext, InSkillItem);
	return HandleSkillReleaseInput(Skill, Skill->GetDirectReleaseInputState(), SkillContext);
}

bool ULxSkillCastModule::StartUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
{
	if (!InSkillItem || !InSkillItem->ItemIsValid())
	{
		return false;
	}

	ULxSkill* Skill = InSkillItem->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	const FLxSkillCastContext SkillContext = NormalizeCastContext(InCastContext, InSkillItem);
	const bool bHandled = HandleSkillReleaseInput(Skill, ELxSkillReleaseInputState::Start, SkillContext);
	if (bHandled && Skill->CanSkillCharge())
	{
		ChargingSkillItem = InSkillItem;
	}
	else if (bHandled && Skill->IsSustainedReleaseSkill())
	{
		SustainedSkillItem = InSkillItem;
	}
	return bHandled;
}

bool ULxSkillCastModule::EndUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
{
	ULxSkillItem* SkillItemToEnd = InSkillItem
		? InSkillItem
		: (SustainedSkillItem ? SustainedSkillItem.Get() : ChargingSkillItem.Get());
	if (!SkillItemToEnd || !SkillItemToEnd->ItemIsValid())
	{
		return false;
	}

	ULxSkill* Skill = SkillItemToEnd->GetOrCreateSkillObject();
	if (!Skill)
	{
		return false;
	}

	return HandleSkillReleaseInput(Skill, ELxSkillReleaseInputState::End, NormalizeCastContext(InCastContext, SkillItemToEnd));
}

FLxSkillCastContext ULxSkillCastModule::NormalizeCastContext(const FLxSkillCastContext& InCastContext, UObject* SourceObject) const
{
	FLxSkillCastContext Result = InCastContext;
	if (!Result.WorldContextObject)
	{
		Result.WorldContextObject = const_cast<ULxSkillCastModule*>(this);
	}

	if (!Result.CasterActor)
	{
		Result.CasterActor = GetOwner();
	}

	if (!Result.InstigatorController)
	{
		if (const APawn* CasterPawn = Cast<APawn>(Result.CasterActor))
		{
			Result.InstigatorController = CasterPawn->GetController();
		}
		else if (const AActor* CasterActor = Result.CasterActor)
		{
			Result.InstigatorController = CasterActor->GetInstigatorController();
		}
	}

	if (!Result.SourceObject)
	{
		Result.SourceObject = SourceObject ? SourceObject : const_cast<ULxSkillCastModule*>(this);
	}

	if (!Result.bOverrideSpawnTransform && Result.CasterActor)
	{
		if (const ALxBaseCharacter* CasterCharacter = Cast<ALxBaseCharacter>(Result.CasterActor))
		{
			Result.SpawnTransform = CasterCharacter->GetSkillReleaseAnchorTransform();
		}
		else
		{
			Result.SpawnTransform = Result.CasterActor->GetActorTransform();
		}
	}

	if (Result.bHasAimDirection)
	{
		Result.AimDirection = Result.AimDirection.GetSafeNormal();
	}

	return Result;
}
void ULxSkillCastModule::ResetSkillCastState()
{
	ClearAnimationSkillRelease(false);
	CurrentCastingSkill = nullptr;
	ChargingSkill = nullptr;
	ChargingSkillItem = nullptr;
	SustainedSkill = nullptr;
	SustainedSkillItem = nullptr;
	// 先清空引用再广播空闲状态，允许监听者安全提交下一次释放请求。
	SetSkillCastState(ELxSkillCastState::Idle);
}

void ULxSkillCastModule::SetSkillCastState(const ELxSkillCastState InNewState)
{
	if (SkillCastState == InNewState)
	{
		return;
	}

	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	ULxCharacterBehaviorControlComponent* BehaviorControlComponent = OwnerCharacter
		? OwnerCharacter->GetCharacterBehaviorControlComponent()
		: nullptr;
	const bool bWasCasting = SkillCastState != ELxSkillCastState::Idle;
	const bool bWillCast = InNewState != ELxSkillCastState::Idle;

	if (BehaviorControlComponent)
	{
		switch (SkillCastState)
		{
		case ELxSkillCastState::DirectReleasing:
			BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Casting);
			break;
		case ELxSkillCastState::Charging:
			BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Charging);
			break;
		case ELxSkillCastState::SustainedReleasing:
			BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Sustaining);
			BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Casting);
			break;
		default:
			break;
		}
	}

	SkillCastState = InNewState;
	if (!BehaviorControlComponent)
	{
		BroadcastModuleDataChanged();
		return;
	}

	switch (SkillCastState)
	{
	case ELxSkillCastState::DirectReleasing:
		BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Casting);
		break;
	case ELxSkillCastState::Charging:
		BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Charging);
		break;
	case ELxSkillCastState::SustainedReleasing:
		BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Sustaining);
		BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Casting);
		break;
	default:
		break;
	}

	if (!bWasCasting && bWillCast)
	{
		BehaviorControlComponent->SetDesiredFacingDirection(CurrentCastContext.AimDirection);
		BehaviorControlComponent->AddFacingControlRequest();
	}
	else if (bWasCasting && !bWillCast)
	{
		BehaviorControlComponent->RemoveFacingControlRequest();
	}
	BroadcastModuleDataChanged();
}

void ULxSkillCastModule::BeginSustainedAimTracking()
{
	ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	SustainedAimComponent = PlayerCharacter ? PlayerCharacter->GetPlayerAimComponent() : nullptr;
	if (!SustainedAimComponent)
	{
		return;
	}

	if (!SustainedAimComponent->OnAimResultChanged.IsBoundToObject(this))
	{
		SustainedAimComponent->OnAimResultChanged.AddUObject(this, &ULxSkillCastModule::HandleAimResultChanged);
	}
	SustainedAimComponent->AddAimResultUpdateRequest();
}

void ULxSkillCastModule::EndSustainedAimTracking()
{
	if (!SustainedAimComponent)
	{
		return;
	}

	SustainedAimComponent->OnAimResultChanged.RemoveAll(this);
	SustainedAimComponent->RemoveAimResultUpdateRequest();
	SustainedAimComponent = nullptr;
}

void ULxSkillCastModule::HandleAimResultChanged(const FLxPlayerAimResult& AimResult)
{
	if (!SustainedSkill || AimResult.SkillDirection.IsNearlyZero())
	{
		return;
	}

	CurrentCastContext.TargetActor = AimResult.TargetActor;
	CurrentCastContext.AimLocation = AimResult.AimLocation;
	CurrentCastContext.bHasAimLocation = true;
	CurrentCastContext.AimDirection = AimResult.SkillDirection;
	CurrentCastContext.bHasAimDirection = true;
	CurrentCastContext.SpawnTransform = FTransform(AimResult.SkillDirection.Rotation(), AimResult.ReleaseLocation);
	if (const ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner()))
	{
		if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
			OwnerCharacter->GetCharacterBehaviorControlComponent())
		{
			BehaviorControlComponent->SetDesiredFacingDirection(AimResult.CameraRayDirection);
		}
	}
	SustainedSkill->TryUpdateSustainedReleaseTransform(CurrentCastContext.SpawnTransform);
}

void ULxSkillCastModule::HandleSkillHitEntriesReady(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& SkillEntryPackages, const TArray<AActor*>& HitTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	ULxCharacterEffectProcessModule* EffectProcessComponent = OwnerCharacter->GetCharacterEffectProcessComponent();
	if (EffectProcessComponent == nullptr)
	{
		return;
	}

	EffectProcessComponent->ProcessSkillHitEffects(SourceSkill, SkillEntryPackages, HitTargets);
}

void ULxSkillCastModule::HandlePersistentSkillHitEntriesReady(ULxSkill* SourceSkill,
	ALxSkillUnitActor* SourceSkillUnit, const TArray<FLxSkillEntryPackage>& SkillEntryPackages,
	const TArray<AActor*>& HitTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterEffectProcessComponent())
	{
		return;
	}
	OwnerCharacter->GetCharacterEffectProcessComponent()->ProcessSkillHitEffects(SourceSkill,
		SkillEntryPackages, HitTargets, true, SourceSkillUnit);
}

void ULxSkillCastModule::HandleSkillEffectsRemoved(ULxSkill* SourceSkill,
	ALxSkillUnitActor* SourceSkillUnit, const TArray<AActor*>& EffectTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterEffectProcessComponent())
	{
		return;
	}
	OwnerCharacter->GetCharacterEffectProcessComponent()->RemovePersistentSkillEffects(SourceSkillUnit, EffectTargets);
}
