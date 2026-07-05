#include "LxSkillCastComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "LxARPG/LxSource/Model/Aim/LxPlayerAimComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillItem.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
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

	SkillCastState = ELxSkillCastState::DirectReleasing;
	CurrentCastingSkill = InSkill;
	if (!InSkill->TryReleaseSkillDirectly())
	{
		ResetSkillCastState();
		return false;
	}

	if (!InSkill->ShouldHoldReleaseStateUntilExplicitFinish())
	{
		ResetSkillCastState();
	}
	return true;
}

bool ULxSkillCastComponent::StartSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	SkillCastState = ELxSkillCastState::Charging;
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
		|| !SkillToEnd->TryEndSkillCharge())
	{
		return false;
	}

	ChargingSkill = nullptr;
	ChargingSkillItem = nullptr;
	if (SkillToEnd->ShouldHoldReleaseStateUntilExplicitFinish())
	{
		SkillCastState = ELxSkillCastState::DirectReleasing;
	}
	else
	{
		ResetSkillCastState();
	}
	return true;
}

bool ULxSkillCastComponent::StartSustainedRelease(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext)
{
	if (!IsSkillCastIdle() || !InSkill || !InitializeSkillForCast(InSkill, InCastContext))
	{
		return false;
	}

	SkillCastState = ELxSkillCastState::SustainedReleasing;
	CurrentCastingSkill = InSkill;
	SustainedSkill = InSkill;
	if (!InSkill->TryStartSustainedRelease())
	{
		ResetSkillCastState();
		return false;
	}

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

	const bool bStopped = SkillToStop->TryStopSustainedRelease();
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

	const bool bCancelled = SkillToCancel->TryCancelSustainedRelease();
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
		bCancelled = SkillCastState == ELxSkillCastState::SustainedReleasing
			? SkillToCancel->TryCancelSustainedRelease()
			: SkillToCancel->TryCancelSkillRelease();
	}

	EndSustainedAimTracking();
	ResetSkillCastState();
	return bCancelled;
}

bool ULxSkillCastComponent::FinishCurrentSkillRelease(ULxSkill* InSkill)
{
	if (IsSkillCastIdle() || InSkill == nullptr || InSkill != CurrentCastingSkill)
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
	SkillCastState = ELxSkillCastState::Idle;
	CurrentCastingSkill = nullptr;
	ChargingSkill = nullptr;
	ChargingSkillItem = nullptr;
	SustainedSkill = nullptr;
	SustainedSkillItem = nullptr;
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
	const TArray<FLxSkillEntryPackage>& SkillEntryPackages, const TArray<AActor*>& HitTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterEffectProcessComponent())
	{
		return;
	}
	OwnerCharacter->GetCharacterEffectProcessComponent()->ProcessSkillHitEffects(SourceSkill,
		SkillEntryPackages, HitTargets, true);
}

void ULxSkillCastComponent::HandleSkillEffectsRemoved(ULxSkill* SourceSkill,
	const TArray<AActor*>& EffectTargets)
{
	ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterEffectProcessComponent())
	{
		return;
	}
	OwnerCharacter->GetCharacterEffectProcessComponent()->RemovePersistentSkillEffects(SourceSkill, EffectTargets);
}
