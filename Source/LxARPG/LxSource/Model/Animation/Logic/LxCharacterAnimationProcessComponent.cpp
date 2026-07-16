#include "LxCharacterAnimationProcessComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "LxCharacterAnimationMotionAnalysisComponent.h"
#include "LxARPG/LxSource/Player/AnimInstance/LxAnimInstanceBase.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterAnimationProcessComponent::ULxCharacterAnimationProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterAnimationProcessComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
	BindMotionAnalysisEvent();
	EnsureAnimationInstanceCached();
	bAnimationProcessInitialized = true;
}

void ULxCharacterAnimationProcessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMotionAnalysisEvent();
	Super::EndPlay(EndPlayReason);
}

void ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal(const FLxCharacterMotionSignal& InMotionSignal)
{
	if (!bAnimationProcessInitialized)
	{
		BaseComponentInitialize();
	}

	CurrentBaseAnimationSignal = ConvertMotionSignalToAnimationSignal(InMotionSignal);
	EnsureAnimationInstanceCached();
	if (AnimInstance)
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
	EnsureAnimationInstanceCached();
	if (AnimInstance)
	{
		AnimInstance->ApplyActionAnimationSignal(CurrentActionAnimationSignal);
	}
}

FLxCharacterAnimationSignal ULxCharacterAnimationProcessComponent::ConvertMotionSignalToAnimationSignal_Implementation(const FLxCharacterMotionSignal& InMotionSignal) const
{
	FLxCharacterAnimationSignal AnimationSignal;
	AnimationSignal.AnimationType = InMotionSignal.MotionType;
	AnimationSignal.bLoop = InMotionSignal.bLoop;
	if (InMotionSignal.MotionSpeed > 10.0f)
	{
		AnimationSignal.PlayRate = FMath::Clamp(InMotionSignal.MotionSpeed / 600.0f, 0.1f, 3.0f);
	}
	else if (InMotionSignal.MotionSpeed > 0.0f)
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

	MotionAnalysisComponent = OwnerCharacter->GetCharacterAnimationMotionAnalysisComponent();
}

void ULxCharacterAnimationProcessComponent::BindMotionAnalysisEvent()
{
	UnbindMotionAnalysisEvent();
	if (MotionAnalysisComponent)
	{
		MotionAnalysisComponent->OnBaseMotionSignalAnalyzed.AddDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal);
		MotionAnalysisComponent->OnActionMotionSignalAnalyzed.AddDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveActionMotionSignal);
	}
}

void ULxCharacterAnimationProcessComponent::UnbindMotionAnalysisEvent()
{
	if (MotionAnalysisComponent)
	{
		MotionAnalysisComponent->OnBaseMotionSignalAnalyzed.RemoveDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveBaseMotionSignal);
		MotionAnalysisComponent->OnActionMotionSignalAnalyzed.RemoveDynamic(this, &ULxCharacterAnimationProcessComponent::ReceiveActionMotionSignal);
	}
}

void ULxCharacterAnimationProcessComponent::EnsureAnimationInstanceCached()
{
	if (AnimInstance)
	{
		return;
	}

	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr || OwnerCharacter->GetMesh() == nullptr)
	{
		return;
	}

	AnimInstance = Cast<ULxAnimInstanceBase>(OwnerCharacter->GetMesh()->GetAnimInstance());
}
