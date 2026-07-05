#include "LxAuraEffectSkillUnitActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/CharacterPoint/Logic/LxCharacterAnchorPointComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ALxAuraEffectSkillUnitActor::ALxAuraEffectSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DetectionComponent = CreateDefaultSubobject<ULxSkillDetectionComponent>(TEXT("DetectionComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
}

bool ALxAuraEffectSkillUnitActor::InitializeAuraEffect(ALxBaseCharacter* InAuraOwner,
	const FLxSkillAuraEffectSpec& InAuraEffectSpec)
{
	const bool bDurationValid = FMath::IsNearlyEqual(InAuraEffectSpec.Duration, -1.0f)
		|| InAuraEffectSpec.Duration > 0.0f;
	if (!IsValid(InAuraOwner) || !IsValid(InAuraOwner->GetAuraEffectAnchorPoint()) || !bDurationValid)
	{
		return false;
	}

	if (AuraOwner && AuraOwner != InAuraOwner)
	{
		AuraOwner->OnDestroyed.RemoveDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed);
	}

	AuraOwner = InAuraOwner;
	AuraEffectSpec = InAuraEffectSpec;
	SkillUnitSpec.LifeSpec.Duration = AuraEffectSpec.Duration > 0.0f ? AuraEffectSpec.Duration : 0.0f;
	SetOwner(AuraOwner);
	SetInstigator(AuraOwner);
	if (!AttachToComponent(AuraOwner->GetAuraEffectAnchorPoint(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale))
	{
		AuraOwner = nullptr;
		return false;
	}
	SetActorRelativeTransform(FTransform::Identity);
	AuraOwner->OnDestroyed.AddUniqueDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed);
	ApplySkillUnitSpecToComponents();
	return true;
}

UPrimitiveComponent* ALxAuraEffectSkillUnitActor::ResolveAuraDetectionComponent_Implementation() const
{
	return FindComponentByClass<UPrimitiveComponent>();
}

void ALxAuraEffectSkillUnitActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AuraOwner)
	{
		AuraOwner->OnDestroyed.RemoveDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed);
	}
	Super::EndPlay(EndPlayReason);
}

void ALxAuraEffectSkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (IsSkillUnitActive() || !CanActivateAuraEffect())
	{
		return;
	}

	AuraDetectionComponent = ResolveAuraDetectionComponent();
	if (!IsValid(AuraDetectionComponent))
	{
		return;
	}

	DetectionComponent->SetTriggerCollisionComponent(AuraDetectionComponent);
	bEndingAuraActivation = false;
	Super::ActivateSkillUnit_Implementation();
	if (IsSkillUnitActive())
	{
		HandleAuraEffectActivated();
	}
}

void ALxAuraEffectSkillUnitActor::StopSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::AuraStopped,
			ELxSkillUnitResultType::Completed, true);
	}
}

void ALxAuraEffectSkillUnitActor::CancelSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::AuraCancelled,
			ELxSkillUnitResultType::Cancelled, false);
	}
}

void ALxAuraEffectSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	SkillUnitSpec.LifeSpec.Duration = AuraEffectSpec.Duration > 0.0f ? AuraEffectSpec.Duration : 0.0f;
	Super::ApplySkillUnitSpecToComponents();
	if (DetectionComponent)
	{
		DetectionComponent->SetTriggerCollisionComponent(AuraDetectionComponent);
	}
}

void ALxAuraEffectSkillUnitActor::BindSkillUnitComponentEvents()
{
	Super::BindSkillUnitComponentEvents();
	if (DetectionComponent)
	{
		DetectionComponent->OnDetectionResult.AddUniqueDynamic(this, &ALxAuraEffectSkillUnitActor::HandleAuraDetectionResult);
	}
}

void ALxAuraEffectSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState,
	ELxSkillAbilityComponentState NewState)
{
	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::DurationExpired,
			ELxSkillUnitResultType::Expired, true);
	}
}

bool ALxAuraEffectSkillUnitActor::CanActivateAuraEffect() const
{
	return IsValid(AuraOwner) && (FMath::IsNearlyEqual(AuraEffectSpec.Duration, -1.0f) || AuraEffectSpec.Duration > 0.0f);
}

void ALxAuraEffectSkillUnitActor::HandleAuraEffectActivated()
{
}

void ALxAuraEffectSkillUnitActor::HandleAuraEffectDeactivated(ELxAuraTargetEffectRemoveReason RemoveReason)
{
}

void ALxAuraEffectSkillUnitActor::HandleAuraDetectionResult(const FLxSkillDetectionResult& DetectionResult)
{
}

void ALxAuraEffectSkillUnitActor::ScanCurrentAuraTargets()
{
	if (!IsSkillUnitActive() || !AuraDetectionComponent || !DetectionComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	AuraDetectionComponent->GetOverlappingActors(OverlappingActors, ALxBaseCharacter::StaticClass());
	OverlappingActors.Remove(AuraOwner);
	DetectionComponent->PublishManualDetectionResult(OverlappingActors);
}

void ALxAuraEffectSkillUnitActor::TriggerAuraTargetHit(AActor* HitTarget)
{
	if (!IsSkillUnitActive() || !IsValid(HitTarget) || HitTarget == AuraOwner || !TriggerComponent)
	{
		return;
	}

	FLxSkillDetectionResult DetectionResult;
	DetectionResult.EventType = ELxSkillDetectionEventType::HitTarget;
	DetectionResult.SourceComponent = TriggerComponent;
	DetectionResult.SourceUnit = this;
	DetectionResult.HitActor = HitTarget;
	DetectionResult.CandidateTargets.Add(HitTarget);
	DetectionResult.HitLocation = HitTarget->GetActorLocation();
	TriggerComponent->HandleDetectionResult(DetectionResult);
}

void ALxAuraEffectSkillUnitActor::EndAuraActivation(ELxAuraTargetEffectRemoveReason RemoveReason,
	ELxSkillUnitResultType ResultType, bool bSuccess)
{
	if (!IsSkillUnitActive() || bEndingAuraActivation)
	{
		return;
	}
	bEndingAuraActivation = true;

	HandleAuraEffectDeactivated(RemoveReason);
	StopSkillUnitComponents();
	FinishSkillUnit(MakeSkillUnitResult(ResultType, bSuccess));
	bEndingAuraActivation = false;
}

void ALxAuraEffectSkillUnitActor::HandleAuraOwnerDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != AuraOwner)
	{
		return;
	}

	if (IsSkillUnitActive())
	{
		EndAuraActivation(ELxAuraTargetEffectRemoveReason::AuraCancelled,
			ELxSkillUnitResultType::Cancelled, false);
	}
	Destroy();
}