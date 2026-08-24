#include "LxSkillCastComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/Aim/LxPlayerAimComponent.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxSkill.h"

ULxSkillCastComponent::ULxSkillCastComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULxSkillCastComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	CurrentCastContext = MakeSkillCastContext(this);
}

void ULxSkillCastComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelCurrentSkillRelease();
	EndSustainedAimTracking();
	Super::EndPlay(EndPlayReason);
}

FLxSkillCastContext ULxSkillCastComponent::MakeSkillCastContext(UObject* SourceObject, AActor* TargetActor,
	FVector AimLocation, bool bHasAimLocation, FVector AimDirection, bool bHasAimDirection) const
{
	FLxSkillCastContext CastContext;
	CastContext.WorldContextObject = const_cast<ULxSkillCastComponent*>(this);
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

bool ULxSkillCastComponent::InitializeSkillForCast(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!InSkill)
	{
		return false;
	}

	CurrentCastContext = NormalizeCastContext(InCastContext);
	InSkill->OnSkillHitEntriesReady.RemoveDynamic(this, &ULxSkillCastComponent::HandleSkillHitEntriesReady);
	InSkill->OnSkillHitEntriesReady.AddDynamic(this, &ULxSkillCastComponent::HandleSkillHitEntriesReady);
	InSkill->OnPersistentSkillHitEntriesReady.RemoveDynamic(this, &ULxSkillCastComponent::HandlePersistentSkillHitEntriesReady);
	InSkill->OnPersistentSkillHitEntriesReady.AddDynamic(this, &ULxSkillCastComponent::HandlePersistentSkillHitEntriesReady);
	InSkill->OnSkillEffectsRemoved.RemoveDynamic(this, &ULxSkillCastComponent::HandleSkillEffectsRemoved);
	InSkill->OnSkillEffectsRemoved.AddDynamic(this, &ULxSkillCastComponent::HandleSkillEffectsRemoved);
	InSkill->PrepareSkillForCast(CurrentCastContext);
	return true;
}

bool ULxSkillCastComponent::ReleaseSkillDirectly(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
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

	BeginTimedSkillRelease(InSkill, ELxPendingSkillReleaseExecution::Direct);
	return true;
}

bool ULxSkillCastComponent::StartSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	SetSkillCastState(ELxSkillCastState::Charging);
	CurrentCastingSkill = InSkill;
	ChargingSkill = InSkill;
	ChargingSkillItem = nullptr;
	if (!InSkill->TryStartSkillCharge())
	{
		ResetSkillCastState();
		return false;
	}

	return true;
}

bool ULxSkillCastComponent::EndSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
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
	BeginTimedSkillRelease(SkillToEnd, ELxPendingSkillReleaseExecution::Charge);
	return true;
}

bool ULxSkillCastComponent::StartSustainedRelease(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
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

	BeginTimedSkillRelease(InSkill, ELxPendingSkillReleaseExecution::Sustained);
	BeginSustainedAimTracking();
	return true;
}

bool ULxSkillCastComponent::StopSustainedRelease(ULxSkill* InSkill)
{
	ULxSkill* SkillToStop = InSkill ? InSkill : SustainedSkill.Get();
	if (SkillCastState != ELxSkillCastState::SustainedReleasing || !SkillToStop
		|| SkillToStop != CurrentCastingSkill || SkillToStop != SustainedSkill)
	{
		return false;
	}

	ClearTimedSkillRelease(true);
	const bool bStopped = SkillToStop->TryStopSustainedRelease();
	MulticastStopSkillActionAnimation();
	EndSustainedAimTracking();
	ResetSkillCastState();
	return bStopped;
}

bool ULxSkillCastComponent::CancelSustainedRelease(ULxSkill* InSkill)
{
	ULxSkill* SkillToCancel = InSkill ? InSkill : SustainedSkill.Get();
	if (SkillCastState != ELxSkillCastState::SustainedReleasing || !SkillToCancel
		|| SkillToCancel != CurrentCastingSkill || SkillToCancel != SustainedSkill)
	{
		return false;
	}

	ClearTimedSkillRelease(true);
	const bool bCancelled = SkillToCancel->TryCancelSustainedRelease();
	MulticastStopSkillActionAnimation();
	EndSustainedAimTracking();
	ResetSkillCastState();
	return bCancelled;
}

bool ULxSkillCastComponent::CancelCurrentSkillRelease()
{
	if (IsSkillCastIdle())
	{
		return false;
	}

	ULxSkill* SkillToCancel = CurrentCastingSkill.Get();
	bool bCancelled = false;
	if (SkillToCancel)
	{
		ClearTimedSkillRelease(true);
		bCancelled = SkillCastState == ELxSkillCastState::SustainedReleasing
			? SkillToCancel->TryCancelSustainedRelease()
			: SkillToCancel->TryCancelSkillRelease();
		MulticastStopSkillActionAnimation();
	}

	EndSustainedAimTracking();
	ResetSkillCastState();
	return bCancelled;
}

bool ULxSkillCastComponent::FinishCurrentSkillRelease(ULxSkill* InSkill)
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

bool ULxSkillCastComponent::HandleSkillReleaseInput(ULxSkill* InSkill, ELxSkillReleaseInputState InInputState, const FLxSkillCastContext& InCastContext)
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
		ServerHandleSkillItemReleaseInput(SkillItemIDTag, InInputState, SkillContext.TargetActor,
			SkillContext.AimLocation, SkillContext.bHasAimLocation,
			SkillContext.AimDirection, SkillContext.bHasAimDirection);
		return true;
	}

	return HandleSkillReleaseInputAuthority(InSkill, InInputState, InCastContext);
}

bool ULxSkillCastComponent::HandleSkillReleaseInputAuthority(ULxSkill* InSkill,
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

void ULxSkillCastComponent::ServerHandleSkillItemReleaseInput_Implementation(FGameplayTag InSkillItemIDTag,
	ELxSkillReleaseInputState InInputState, AActor* InTargetActor, FVector_NetQuantize InAimLocation,
	bool bInHasAimLocation, FVector_NetQuantizeNormal InAimDirection, bool bInHasAimDirection)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	ULxSkillBackpackComponent* SkillBackpack = OwnerCharacter ? OwnerCharacter->GetSkillBackpackComponent() : nullptr;
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

void ULxSkillCastComponent::MulticastPlaySkillActionAnimation_Implementation(float InSkillReleaseDuration)
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
	ActionMotionSignal.MotionType = ELxCharacterMotionType::Skill;
	// 技能动作资产统一按一秒制作，通过播放速率把实际时长拉伸或压缩到技能释放时间。
	ActionMotionSignal.MotionSpeed = 1.0f / FMath::Max(InSkillReleaseDuration, 0.1f);
	ActionMotionSignal.bLoop = false;
	BehaviorControlComponent->SendActionAnimationMotionSignal(ActionMotionSignal);
}

void ULxSkillCastComponent::MulticastStopSkillActionAnimation_Implementation()
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
	ActionMotionSignal.bLoop = false;
	BehaviorControlComponent->SendActionAnimationMotionSignal(ActionMotionSignal);
}

void ULxSkillCastComponent::BeginTimedSkillRelease(ULxSkill* InSkill,
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

	ClearTimedSkillRelease(false);
	PendingSkillReleaseExecution = InExecutionType;
	const float ReleaseDuration = InSkill->GetSkillReleaseDuration();
	MulticastPlaySkillActionAnimation(ReleaseDuration);

	GetWorld()->GetTimerManager().SetTimer(SkillReleaseExecutionTimerHandle, this,
		&ULxSkillCastComponent::ExecuteTimedSkillRelease, ReleaseDuration * 0.5f, false);
	GetWorld()->GetTimerManager().SetTimer(SkillReleaseCompletionTimerHandle, this,
		&ULxSkillCastComponent::CompleteTimedSkillRelease, ReleaseDuration, false);
}

void ULxSkillCastComponent::ExecuteTimedSkillRelease()
{
	ULxSkill* Skill = CurrentCastingSkill.Get();
	if (!Skill)
	{
		ClearTimedSkillRelease(false);
		ResetSkillCastState();
		return;
	}

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

void ULxSkillCastComponent::CompleteTimedSkillRelease()
{
	ULxSkill* Skill = CurrentCastingSkill.Get();
	const ELxPendingSkillReleaseExecution CompletedExecution = PendingSkillReleaseExecution;
	PendingSkillReleaseExecution = ELxPendingSkillReleaseExecution::None;
	if (!Skill)
	{
		ResetSkillCastState();
		return;
	}

	Skill->CompleteSkillReleaseTiming();
	MulticastStopSkillActionAnimation();
	if (CompletedExecution != ELxPendingSkillReleaseExecution::Sustained
		&& !Skill->ShouldHoldReleaseStateUntilExplicitFinish())
	{
		ResetSkillCastState();
	}
}

void ULxSkillCastComponent::ClearTimedSkillRelease(bool bCancelSkillTiming)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkillReleaseExecutionTimerHandle);
		World->GetTimerManager().ClearTimer(SkillReleaseCompletionTimerHandle);
	}

	if (bCancelSkillTiming && CurrentCastingSkill)
	{
		CurrentCastingSkill->CancelSkillReleaseTiming();
	}
	PendingSkillReleaseExecution = ELxPendingSkillReleaseExecution::None;
}

FGameplayTag ULxSkillCastComponent::ResolveSkillItemIDTag(const ULxSkill* InSkill) const
{
	ULxSkillItem* SkillItem = InSkill ? Cast<ULxSkillItem>(InSkill->GetOuter()) : nullptr;
	return SkillItem && SkillItem->ItemIsValid() ? SkillItem->ItemIDTag() : FGameplayTag();
}

bool ULxSkillCastComponent::ReleaseSkillItemDirectly(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
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

bool ULxSkillCastComponent::StartUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
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

bool ULxSkillCastComponent::EndUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext)
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

FLxSkillCastContext ULxSkillCastComponent::NormalizeCastContext(const FLxSkillCastContext& InCastContext, UObject* SourceObject) const
{
	FLxSkillCastContext Result = InCastContext;
	if (!Result.WorldContextObject)
	{
		Result.WorldContextObject = const_cast<ULxSkillCastComponent*>(this);
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
		Result.SourceObject = SourceObject ? SourceObject : const_cast<ULxSkillCastComponent*>(this);
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
void ULxSkillCastComponent::ResetSkillCastState()
{
	ClearTimedSkillRelease(false);
	SetSkillCastState(ELxSkillCastState::Idle);
	CurrentCastingSkill = nullptr;
	ChargingSkill = nullptr;
	ChargingSkillItem = nullptr;
	SustainedSkill = nullptr;
	SustainedSkillItem = nullptr;
}

void ULxSkillCastComponent::SetSkillCastState(const ELxSkillCastState InNewState)
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
}

void ULxSkillCastComponent::BeginSustainedAimTracking()
{
	ALxPlayerCharacter* PlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	SustainedAimComponent = PlayerCharacter ? PlayerCharacter->GetPlayerAimComponent() : nullptr;
	if (!SustainedAimComponent)
	{
		return;
	}

	SustainedAimComponent->OnAimResultChanged.AddUniqueDynamic(this, &ULxSkillCastComponent::HandleAimResultChanged);
	SustainedAimComponent->AddAimResultUpdateRequest();
}

void ULxSkillCastComponent::EndSustainedAimTracking()
{
	if (!SustainedAimComponent)
	{
		return;
	}

	SustainedAimComponent->OnAimResultChanged.RemoveDynamic(this, &ULxSkillCastComponent::HandleAimResultChanged);
	SustainedAimComponent->RemoveAimResultUpdateRequest();
	SustainedAimComponent = nullptr;
}

void ULxSkillCastComponent::HandleAimResultChanged(const FLxPlayerAimResult& AimResult)
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

void ULxSkillCastComponent::HandleSkillHitEntriesReady(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& SkillEntryPackages, const TArray<AActor*>& HitTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	ULxCharacterEffectProcessComponent* EffectProcessComponent = OwnerCharacter->GetCharacterEffectProcessComponent();
	if (EffectProcessComponent == nullptr)
	{
		return;
	}

	EffectProcessComponent->ProcessSkillHitEffects(SourceSkill, SkillEntryPackages, HitTargets);
}

void ULxSkillCastComponent::HandlePersistentSkillHitEntriesReady(ULxSkill* SourceSkill,
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

void ULxSkillCastComponent::HandleSkillEffectsRemoved(ULxSkill* SourceSkill,
	ALxSkillUnitActor* SourceSkillUnit, const TArray<AActor*>& EffectTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterEffectProcessComponent())
	{
		return;
	}
	OwnerCharacter->GetCharacterEffectProcessComponent()->RemovePersistentSkillEffects(SourceSkillUnit, EffectTargets);
}
