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
		BehaviorControlComponent->OnBehaviorStateChanged.AddDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveBehaviorStateChanged);
		BehaviorControlComponent->OnBaseMotionSignalChanged.AddDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal);
		BehaviorControlComponent->OnActionMotionSignalChanged.AddDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveActionMotionSignal);
		BehaviorControlComponent->ResendCurrentBaseAnimationMotionSignal();
	}
}

void ULxCharacterAnimationProcessComponent::UnbindBehaviorControlEvents()
{
	if (BehaviorControlComponent)
	{
		BehaviorControlComponent->OnBehaviorStateChanged.RemoveDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveBehaviorStateChanged);
		BehaviorControlComponent->OnBaseMotionSignalChanged.RemoveDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal);
		BehaviorControlComponent->OnActionMotionSignalChanged.RemoveDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveActionMotionSignal);
	}
}

bool ULxCharacterAnimationProcessComponent::EnsureAnimationInstanceCached()
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	ULxAnimInstanceBase* NewInstance = OwnerCharacter && OwnerCharacter->GetMesh()
		? Cast<ULxAnimInstanceBase>(OwnerCharacter->GetMesh()->GetAnimInstance()) : nullptr;
	if (AnimInstance != NewInstance)
	{
		AnimInstance = NewInstance;
		if (AnimInstance)
		{
			AnimInstance->ApplyBaseAnimationSignal(CurrentBaseAnimationSignal);
			AnimInstance->ApplyActionAnimationSignal(CurrentActionAnimationSignal);
			return true;
		}
	}
	return false;
}
