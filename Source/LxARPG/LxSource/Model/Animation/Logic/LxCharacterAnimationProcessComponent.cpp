#include "LxCharacterAnimationProcessComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterAnimationProcessComponent::ULxCharacterAnimationProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterAnimationProcessComponent::BaseComponentInitialize()
{
	if (bAnimationProcessInitialized || bAnimationProcessInitializing)
	{
		return;
	}

	TGuardValue<bool> InitializationGuard(bAnimationProcessInitializing, true);
	CacheOwnerComponents();
	EnsureAnimationInstanceCached();
	bAnimationProcessInitialized = true;
	BindBehaviorControlEvents();
}

void ULxCharacterAnimationProcessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AnimInstance) AnimInstance->OnAnimationEvent.RemoveAll(this);
	SetSkillAnimationTickRequired(false);
	UnbindBehaviorControlEvents();
	Super::EndPlay(EndPlayReason);
}

void ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal(const FLxCharacterMotionSignal& InMotionSignal)
{
	if (!bAnimationProcessInitialized)
	{
		BaseComponentInitialize();
	}

	CurrentBaseAnimationSignal = ConvertMotionSignalToAnimationSignal(InMotionSignal);
	if (!EnsureAnimationInstanceCached() && AnimInstance)
	{
		AnimInstance->ApplyBaseAnimationSignal(CurrentBaseAnimationSignal);
	}
}

void ULxCharacterAnimationProcessComponent::ReceiveActionMotionSignal(const FLxCharacterMotionSignal& InMotionSignal)
{
	if (!bAnimationProcessInitialized)
	{
		BaseComponentInitialize();
	}

	CurrentActionAnimationSignal = ConvertMotionSignalToAnimationSignal(InMotionSignal);
	CurrentActionAnimationSignal.SkillId = InMotionSignal.SkillId;
	// 蓝图只负责转换表现参数，不能丢失权威端的释放身份。
	CurrentActionAnimationSignal.CastId = InMotionSignal.CastId;
	SetSkillAnimationTickRequired(InMotionSignal.CastId.IsValid());
	if (!EnsureAnimationInstanceCached() && AnimInstance)
	{
		AnimInstance->ApplyActionAnimationSignal(CurrentActionAnimationSignal);
	}
}

void ULxCharacterAnimationProcessComponent::ReceiveBehaviorStateChanged(const FGameplayTag InBehaviorStateTag,
	const bool bInActive)
{
	if (!InBehaviorStateTag.IsValid())
	{
		return;
	}
	if (bInActive)
	{
		CurrentBehaviorStateTags.AddTag(InBehaviorStateTag);
	}
	else
	{
		CurrentBehaviorStateTags.RemoveTag(InBehaviorStateTag);
	}
	OnDataChange.Broadcast();
}

FLxCharacterAnimationSignal ULxCharacterAnimationProcessComponent::ConvertMotionSignalToAnimationSignal_Implementation(const FLxCharacterMotionSignal& InMotionSignal) const
{
	FLxCharacterAnimationSignal AnimationSignal;
	AnimationSignal.AnimationType = InMotionSignal.MotionType;
	AnimationSignal.SkillId = InMotionSignal.SkillId;
	AnimationSignal.CastId = InMotionSignal.CastId;
	AnimationSignal.bLoop = InMotionSignal.bLoop;
	if (InMotionSignal.MotionType == ELxCharacterMotionType::Move
		|| InMotionSignal.MotionType == ELxCharacterMotionType::Run
		|| InMotionSignal.MotionType == ELxCharacterMotionType::MediumMove)
	{
		AnimationSignal.PlayRate = FMath::Clamp(InMotionSignal.MotionSpeed / 600.0f, 0.1f, 3.0f);
	}
	else if (InMotionSignal.MotionSpeed > 0.0f && InMotionSignal.MotionSpeed <= 10.0f)
	{
		AnimationSignal.PlayRate = InMotionSignal.MotionSpeed;
	}
	else
	{
		AnimationSignal.PlayRate = 1.0f;
	}
	return AnimationSignal;
}

void ULxCharacterAnimationProcessComponent::CacheOwnerComponents()
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	BehaviorControlComponent = OwnerCharacter->GetCharacterBehaviorControlComponent();
}

void ULxCharacterAnimationProcessComponent::BindBehaviorControlEvents()
{
	UnbindBehaviorControlEvents();
	if (BehaviorControlComponent)
	{
		CurrentBehaviorStateTags = BehaviorControlComponent->GetActiveBehaviorStateTags();
		BehaviorControlComponent->OnBehaviorStateChanged.AddUObject(this, &ULxCharacterAnimationProcessComponent::ReceiveBehaviorStateChanged);
		BehaviorControlComponent->OnBaseMotionSignalChanged.AddUObject(this, &ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal);
		BehaviorControlComponent->OnActionMotionSignalChanged.AddUObject(this, &ULxCharacterAnimationProcessComponent::ReceiveActionMotionSignal);
		BehaviorControlComponent->ResendCurrentBaseAnimationMotionSignal();
	}
}

void ULxCharacterAnimationProcessComponent::UnbindBehaviorControlEvents()
{
	if (BehaviorControlComponent)
	{
		BehaviorControlComponent->OnBehaviorStateChanged.RemoveAll(this);
		BehaviorControlComponent->OnBaseMotionSignalChanged.RemoveAll(this);
		BehaviorControlComponent->OnActionMotionSignalChanged.RemoveAll(this);
	}
}

bool ULxCharacterAnimationProcessComponent::EnsureAnimationInstanceCached()
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	ULxAnimInstanceBase* NewInstance = OwnerCharacter && OwnerCharacter->GetMesh()
		? Cast<ULxAnimInstanceBase>(OwnerCharacter->GetMesh()->GetAnimInstance()) : nullptr;
	if (AnimInstance != NewInstance)
	{
		if (AnimInstance) AnimInstance->OnAnimationEvent.RemoveAll(this);
		AnimInstance = NewInstance;
		if (AnimInstance)
		{
			AnimInstance->OnAnimationEvent.AddUObject(this, &ULxCharacterAnimationProcessComponent::HandleAnimationEvent);
			AnimInstance->ApplyBaseAnimationSignal(CurrentBaseAnimationSignal);
			AnimInstance->ApplyActionAnimationSignal(CurrentActionAnimationSignal);
			return true;
		}
	}
	return false;
}

void ULxCharacterAnimationProcessComponent::HandleAnimationEvent(const FLxCharacterAnimationEvent& Event)
{
	OnAnimationEvent.Broadcast(Event);
}

void ULxCharacterAnimationProcessComponent::SetSkillAnimationTickRequired(bool bRequired)
{
	const ALxBaseCharacter* Character = GetCharacterOwner();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || !Character->HasAuthority() || bRequired == bSkillAnimationTickRequired) return;
	if (bRequired)
	{
		SavedVisibilityTickOption = static_cast<uint8>(Mesh->VisibilityBasedAnimTickOption);
		bSavedUpdateRateOptimizations = Mesh->bEnableUpdateRateOptimizations;
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Mesh->bEnableUpdateRateOptimizations = false;
	}
	else
	{
		Mesh->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(SavedVisibilityTickOption);
		Mesh->bEnableUpdateRateOptimizations = bSavedUpdateRateOptimizations;
	}
	bSkillAnimationTickRequired = bRequired;
}
